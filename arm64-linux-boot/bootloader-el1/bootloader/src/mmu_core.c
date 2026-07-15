#include <stdint.h>  // 引入标准整数类型 (如 uint64_t, uint32_t)
#include "mmu.h"     // 引入对外的 API 声明和 mmu_region_t 结构体定义

/* =====================================================================
 * 宏定义区：ARM64 硬件架构的数学契约
 * ===================================================================== */

// 页表容量定义 (9位索引 = 512个条目)
#define MMU_L1_ENTRY_COUNT   512U    // 一级(L1)页表有 512 个槽位
#define MMU_L2_ENTRY_COUNT   512U    // 二级(L2)页表也有 512 个槽位
#define MMU_MAX_L2_TABLES    8U      // 我们预先分配 8 张 L2 页表 (即最大支持映射 8GB 空间)

// 内存块大小定义
#define MMU_L1_BLOCK_SIZE    0x40000000ULL   /* 1GB (2^30)：L1 每一个条目管辖的空间大小 */
#define MMU_L2_BLOCK_SIZE    0x00200000ULL   /* 2MB (2^21)：L2 每一个条目管辖的空间大小 */
#define MMU_TABLE_ALIGN      0x1000ULL       /* 4KB (4096)：硬件规定页表基址必须 4KB 对齐 */
#define MMU_VA_BITS          39U             /* 39位：系统采用 39 位的虚拟地址空间 (Sv39) */

// 页表项 (PTE) 的基本标志位 (Bits [1:0])
#define PTE_VALID            (1ULL << 0)     // Bit 0: 为 1 表示该条目有效，MMU 可以解析它
#define PTE_TABLE            (1ULL << 1)     // Bit 1: 为 1 表示该条目指向下一级页表 (目录)
#define PTE_BLOCK            (0ULL << 1)     // Bit 1: 为 0 表示该条目直接指向物理内存大块 (Block)

// 页表项 (PTE) 的属性控制位
#define PTE_ATTRIDX(x)       ((uint64_t)(x) << 2) // Bits [4:2]: MAIR 寄存器的槽位索引 (0~7)
#define PTE_AP_RW_EL1        (0ULL << 6)          // Bits [7:6]: 00 表示 EL1(内核) 具有读写权限 (RW)
#define PTE_AP_RO_EL1        (2ULL << 6)          // Bits [7:6]: 10 表示 EL1(内核) 仅有只读权限 (RO)
#define PTE_SH_OUTER         (2ULL << 8)          // Bits [9:8]: 10 表示外部共享 (针对 Device 不走 Cache)
#define PTE_SH_INNER         (3ULL << 8)          // Bits [9:8]: 11 表示内部共享 (针对多核 CPU 间的 Cache 一致性)
#define PTE_AF               (1ULL << 10)         // Bit 10: 访问标志位 (Access Flag)，必须置 1，否则触发异常
#define PTE_PXN              (1ULL << 53)         // Bit 53: 特权级执行从不 (Privileged Execute-Never)，禁止内核在此处执行代码
#define PTE_UXN              (1ULL << 54)         // Bit 54: 用户级执行从不 (Unprivileged Execute-Never)，禁止用户态在此处执行代码

// MAIR_EL1 寄存器的 Cache 策略颜料 (8-bit)
#define MAIR_ATTR_DEVICE_nGnRnE  0x00    // 设备内存：不聚集(nG), 不重排序(nR), 不提前写(nE) -> 最严格的实时直写
#define MAIR_ATTR_NORMAL_WBWA    0xFF    // 普通内存：Write-Back (回写) 且 Write-Allocate (写分配) -> 性能最强的 Cache

// 将上述两种“颜料”挤入 MAIR 寄存器的 槽位0 和 槽位1
#define MAIR_VALUE  ((((uint64_t)MAIR_ATTR_DEVICE_nGnRnE) << 0) | \
                     (((uint64_t)MAIR_ATTR_NORMAL_WBWA)   << 8))

// TCR_EL1 (转换控制寄存器) 的配置参数
#define TCR_T0SZ_39BIT       (25ULL << 0)    // T0SZ: 64 - 39 = 25，告知硬件低地址区 (TTBR0) 的大小是 39 位
#define TCR_IRGN0_WBWA       (1ULL << 8)     // 内部 Cache 策略 (页表本身)：Write-Back Write-Allocate
#define TCR_ORGN0_WBWA       (1ULL << 10)    // 外部 Cache 策略 (页表本身)：Write-Back Write-Allocate
#define TCR_SH0_INNER        (3ULL << 12)    // 页表访问的共享属性：内部共享
#define TCR_TG0_4K           (0ULL << 14)    // 颗粒度 (Translation Granule)：4KB 的页表步长
#define TCR_EPD1_DISABLE     (1ULL << 23)    // EPD1: 禁用高地址区 (TTBR1)，因为 Bootloader 用不到高半区
#define TCR_IPS_40BIT        (2ULL << 32)    // IPS: 物理地址空间大小设置为 40位 (最高支持 1TB 物理内存)

// SCTLR_EL1 (系统控制寄存器) 的总闸开关
#define SCTLR_M              (1ULL << 0)     // Bit 0: MMU 硬件开关
#define SCTLR_C              (1ULL << 2)     // Bit 2: Data Cache (数据缓存) 开关
#define SCTLR_I              (1ULL << 12)    // Bit 12: Instruction Cache (指令缓存) 开关

// 地址提取掩码 (极其重要，用于过滤掉杂乱的属性位)
#define TABLE_ADDR_MASK      0x0000FFFFFFFFF000ULL  // 提取 L1/L2 目录的基址 (低 12 位强制清零，满足 4KB 对齐)
#define L2_BLOCK_MASK        0x0000FFFFFFE00000ULL  // 提取 L2 物理块的基址 (低 21 位强制清零，满足 2MB 对齐)


/* =====================================================================
 * 全局数据区：页表内存池 (必须 4KB 对齐并分配在静态区)
 * ===================================================================== */

// L1 根页表，共 512 个槽位 (刚好 4KB)
__attribute__((aligned(4096))) static uint64_t l1_table[MMU_L1_ENTRY_COUNT];

// L2 二级页表池，共 8 张表，每张表 512 个槽位 (总计 32KB)
__attribute__((aligned(4096))) static uint64_t l2_pool[MMU_MAX_L2_TABLES][MMU_L2_ENTRY_COUNT];

// 记录当前已经从 l2_pool 中借出了几张 L2 表
static uint32_t g_used_l2_tables = 0;


/* =====================================================================
 * 内联汇编封装区：底层屏障与系统寄存器操作
 * ===================================================================== */

static inline void dsb_sy(void)
{
    // 数据同步屏障：确保在这条指令之前的所有内存读写操作，都必须在物理上执行完毕
    __asm__ volatile("dsb sy" ::: "memory");
}

static inline void isb(void)
{
    // 指令同步屏障：清空 CPU 流水线，确保下一条指令能看到最新的系统状态 (如 MMU 刚开启)
    __asm__ volatile("isb" ::: "memory");
}

static inline void tlbi_vmalle1(void)
{
    // 使 EL1 下的所有 TLB (地址转换缓存) 全部失效，防止 CPU 读到旧的地址映射
    __asm__ volatile("tlbi vmalle1" ::: "memory");
}

static inline void dc_cvac(uint64_t addr)
{
    // 数据 Cache 操作：Clean (刷出脏数据到内存) 到 PoC (系统一致性观察点)，基于虚拟地址
    __asm__ volatile("dc cvac, %0" :: "r"(addr) : "memory");
}

static inline uint64_t read_ctr_el0(void)
{
    // 读取 Cache 类型寄存器，用于后续动态计算 CPU 的 Cache Line Size
    uint64_t v;
    __asm__ volatile("mrs %0, ctr_el0" : "=r"(v));
    return v;
}

static inline void write_mair_el1(uint64_t v)
{
    // 写入 MAIR_EL1，配置 Cache 的“调色板”
    __asm__ volatile("msr mair_el1, %0" :: "r"(v));
}

static inline void write_tcr_el1(uint64_t v)
{
    // 写入 TCR_EL1，配置地址翻译的规则(如 39 位地址、4KB 颗粒等)
    __asm__ volatile("msr tcr_el1, %0" :: "r"(v));
}

static inline void write_ttbr0_el1(uint64_t v)
{
    // 写入 TTBR0_EL1，将 L1 根页表的物理地址递交给硬件 MMU
    __asm__ volatile("msr ttbr0_el1, %0" :: "r"(v));
}

static inline void write_ttbr1_el1(uint64_t v)
{
    // 写入 TTBR1_EL1，高位地址表，我们不用，所以后续会填 0
    __asm__ volatile("msr ttbr1_el1, %0" :: "r"(v));
}

uint64_t mmu_read_sctlr(void)
{
    // 读取当前系统控制寄存器(总闸)的状态
    uint64_t v;
    __asm__ volatile("mrs %0, sctlr_el1" : "=r"(v));
    return v;
}

static inline void write_sctlr_el1(uint64_t v)
{
    // 写入系统控制寄存器，用于最终开启或关闭 MMU 和 Cache
    __asm__ volatile("msr sctlr_el1, %0" :: "r"(v));
}


/* =====================================================================
 * 核心逻辑区：页表构建与策略翻译
 * ===================================================================== */

// 工具函数：将一段内存数组清零
static void zero_u64(uint64_t *buf, uint32_t count)
{
    uint32_t i;
    for (i = 0; i < count; ++i) {
        buf[i] = 0;
    }
}

// 制作 L1 目录描述符：物理基址 + Valid 标志 + Table 标志 (指明下一级还有表)
static uint64_t make_table_desc(uint64_t table_pa)
{
    return (table_pa & TABLE_ADDR_MASK) | PTE_VALID | PTE_TABLE;
}

// 制作 L2 大块描述符：物理基址 + 属性 + Valid 标志 + Block 标志 (指明这是终点，2MB连续内存)
static uint64_t make_l2_block_desc(uint64_t pa, uint64_t attrs)
{
    return (pa & L2_BLOCK_MASK) | attrs | PTE_VALID | PTE_BLOCK;
}

// 防呆检查：校验平台传入的内存区域参数是否合法
static int mmu_region_validate(const mmu_region_t *r)
{
    uint64_t end;

    // 判空或大小为 0，直接拒绝
    if (r == 0 || r->size == 0) {
        return MMU_ERR_INVAL;
    }

    // 对齐检查：虚拟地址、物理地址、大小，统统必须是 2MB 的整数倍，否则没法做块映射
    if (((r->va | r->pa | r->size) & (MMU_L2_BLOCK_SIZE - 1ULL)) != 0) {
        return MMU_ERR_ALIGN;
    }

    // 越界检查：地址范围不能超过设定的 39 位极限
    end = r->va + r->size - 1ULL;
    if (end >= (1ULL << MMU_VA_BITS)) {
        return MMU_ERR_VA_RANGE;
    }

    return MMU_OK; // 检查通过
}

// 策略转换：把 C 语言里简单的枚举 (Device/Normal) 翻译成 64位 PTE 寄存器的复杂标志位
static uint64_t mmu_make_region_attrs(const mmu_region_t *r)
{
    uint64_t attrs = PTE_AF; // 访问标志默认必须开启

    if (r->attr == MMU_MEM_DEVICE) {
        // 如果是设备：查 MAIR 槽位 0，设为外部共享，强行禁止执行 (防漏洞)
        attrs |= PTE_ATTRIDX(0);
        attrs |= PTE_SH_OUTER;
        attrs |= PTE_PXN | PTE_UXN;
    } else {
        // 如果是普通内存：查 MAIR 槽位 1，设为内部多核共享
        attrs |= PTE_ATTRIDX(1);
        attrs |= PTE_SH_INNER;

        // 如果明确没有 EXEC 可执行权限，才打上防执行烙印
        if ((r->flags & MMU_REGION_EXEC) == 0U) {
            attrs |= PTE_PXN | PTE_UXN;
        }
    }

    // 处理读写权限
    if (r->flags & MMU_REGION_RO) {
        attrs |= PTE_AP_RO_EL1; // 只读
    } else {
        attrs |= PTE_AP_RW_EL1; // 可读写
    }

    return attrs;
}

// 页表动态分配器：根据 L1 索引，获取已有的 L2 表，或者从池子里现捞一个新表
static uint64_t *mmu_get_or_alloc_l2(uint32_t l1_index)
{
    uint64_t desc;
    uint64_t table_pa;
    uint64_t *table;

    desc = l1_table[l1_index]; // 查 L1 表的对应槽位

    // 如果槽位有效，说明之前已经分配过 L2 表了，剥离标志位，直接返回老地址
    if (desc & PTE_VALID) {
        table_pa = desc & TABLE_ADDR_MASK;
        return (uint64_t *)(uintptr_t)table_pa;
    }

    // 如果没分配过，检查内存池超标了没
    if (g_used_l2_tables >= MMU_MAX_L2_TABLES) {
        return 0; // Out of memory
    }

    // 从内存池取出新表，清空它，计数器加一
    table = l2_pool[g_used_l2_tables];
    zero_u64(table, MMU_L2_ENTRY_COUNT);
    g_used_l2_tables++;

    // 将新表的物理基址制作成目录描述符，挂载到 L1 的树枝上！
    l1_table[l1_index] = make_table_desc((uint64_t)(uintptr_t)table);
    
    return table;
}

// 降维切片器：将一个大内存区域，以 2MB 为步长，逐个切片填入页表
static int mmu_map_region(const mmu_region_t *r)
{
    uint64_t attrs;
    uint64_t off;

    // 第一步：严格安检
    int rc = mmu_region_validate(r);
    if (rc != MMU_OK) {
        return rc;
    }

    // 第二步：算好这片区域统一的 PTE 标志位
    attrs = mmu_make_region_attrs(r);

    // 第三步：2MB 步进循环切片
    for (off = 0; off < r->size; off += MMU_L2_BLOCK_SIZE) {
        uint64_t va = r->va + off; // 当前切片的虚拟地址
        uint64_t pa = r->pa + off; // 当前切片的物理基址

        // 神奇的位移：提取最高 9 位得 L1 索引，提取中间 9 位得 L2 索引
        uint32_t l1_index = (uint32_t)((va >> 30) & 0x1FFU);
        uint32_t l2_index = (uint32_t)((va >> 21) & 0x1FFU);
        
        // 拿到这 1GB 大区对应的 L2 表
        uint64_t *l2_table = mmu_get_or_alloc_l2(l1_index);

        if (l2_table == 0) {
            return MMU_ERR_NO_L2_TABLE; // 内存池耗尽
        }

        // 防重叠检查：如果这个 2MB 槽位已经被别人占了，立马报错，防止静默覆盖
        if (l2_table[l2_index] & PTE_VALID) {
            return MMU_ERR_OVERLAP;
        }

        // 将组装好的 64位 物理块描述符，郑重地填入 L2 表的对应格子里
        l2_table[l2_index] = make_l2_block_desc(pa, attrs);
    }

    return MMU_OK;
}


/* =====================================================================
 * 外部公共 API 区：被 boot.c 调用的生命周期函数
 * ===================================================================== */

// MMU 初始化：清空历史，并遍历平台数据建表
int mmu_init(const mmu_region_t *regions, uint32_t count)
{
    uint32_t i;
    int rc;

    // 参数防呆
    if (regions == 0 || count == 0U) {
        return MMU_ERR_INVAL;
    }

    // 将 L1 表和整个 L2 池彻底清零，抹除内存中的随机垃圾数据
    zero_u64(l1_table, MMU_L1_ENTRY_COUNT);
    for (i = 0; i < MMU_MAX_L2_TABLES; ++i) {
        zero_u64(l2_pool[i], MMU_L2_ENTRY_COUNT);
    }
    g_used_l2_tables = 0; // 复位分配器

    // 遍历平台层传来的图纸数组，逐个区域进行映射
    for (i = 0; i < count; ++i) {
        rc = mmu_map_region(&regions[i]);
        if (rc != MMU_OK) {
            return rc; // 一旦出错，停止启动，向上级汇报
        }
    }

    return MMU_OK;
}

// 终极点火：配置寄存器，开启 MMU 和 Cache
void mmu_enable(void)
{
    uint64_t tcr;
    uint64_t sctlr;

    dsb_sy(); // 确保页表数据已经全部真正写入物理内存 DDR 中
    isb();

    // 1. 设置颜料盘 (MAIR)
    write_mair_el1(MAIR_VALUE);

    // 2. 设置寻址规则 (TCR)
    tcr = TCR_T0SZ_39BIT | TCR_IRGN0_WBWA | TCR_ORGN0_WBWA |
          TCR_SH0_INNER | TCR_TG0_4K | TCR_EPD1_DISABLE | TCR_IPS_40BIT;
    write_tcr_el1(tcr);

    // 3. 将我们静态区的 L1 根表基址，报告给硬件寻址首长 (TTBR0)
    write_ttbr0_el1((uint64_t)(uintptr_t)l1_table);
    write_ttbr1_el1(0); // 禁用高半区表

    dsb_sy();
    isb();

    // 4. 洗刷历史遗留的 TLB 缓存，保证从纯净状态开始
    tlbi_vmalle1();
    dsb_sy();
    isb();

    // 5. 拨动总闸 (SCTLR)
    sctlr = mmu_read_sctlr();
    sctlr |= SCTLR_M; // 开 MMU
    sctlr |= SCTLR_I; // 开 I-Cache
    sctlr |= SCTLR_C; // 开 D-Cache

    write_sctlr_el1(sctlr);
    isb(); // 屏障：这句话一旦越过，虚拟世界的魔法立刻生效！
}

// 移交现场复位：关闭 MMU (Linux 启动协议要求)
void mmu_disable(void)
{
    uint64_t sctlr = mmu_read_sctlr();

    dsb_sy();
    isb();

    // 剥夺 MMU 和 Cache 的权限位
    sctlr &= ~SCTLR_C;
    sctlr &= ~SCTLR_I;
    sctlr &= ~SCTLR_M;

    write_sctlr_el1(sctlr); // 重新写入总闸，世界重回物理态
    isb();

    // 关闭后，再次清空 TLB，不给 Linux 内核留后患
    tlbi_vmalle1();
    dsb_sy();
    isb();
}

// 硬件自适应的 Cache 强制冲刷：解决 DTB 脏数据问题
void mmu_dcache_clean_range(uint64_t start, uint64_t size)
{
    uint64_t ctr;
    uint64_t line_size;
    uint64_t addr;
    uint64_t end;

    if (size == 0U) {
        return;
    }

    // 动态探测：读取 CTR_EL0 寄存器，算出这块 CPU 一行 Cache 是多少字节
    ctr = read_ctr_el0();
    line_size = 4ULL << ((ctr >> 16) & 0xFU); // 公式：4 << DminLine

    // 向下取整，确保起始地址与 Cache Line 边界完美对齐
    addr = start & ~(line_size - 1ULL);
    // 算出结束地址
    end  = (start + size + line_size - 1ULL) & ~(line_size - 1ULL);

    // 循环游走，按行 (line_size) 将 Cache 数据硬挤回 DDR 中
    for (; addr < end; addr += line_size) {
        dc_cvac(addr);
    }

    dsb_sy(); // 屏障：确保这些冲刷指令全部物理落地
}

// 获取 L1 表基址 (用于诊断打印)
uint64_t mmu_root_table_pa(void)
{
    return (uint64_t)(uintptr_t)l1_table;
}

// 获取 L2 内存池使用量 (用于诊断打印)
uint32_t mmu_used_l2_tables(void)
{
    return g_used_l2_tables;
}
