#include "l1_data_cache.h" // 包含您提供的 l1_data_cache 类定义
#include <systemc.h>

// 封装后的 SystemC 模块
SC_MODULE(l1_data_cache_sc) {
    // 时钟和复位端口
    sc_in<bool> clk;
    sc_in<bool> rst;

    // 内部 l1_data_cache 对象
    l1_data_cache cache;

    // 构造函数
    SC_CTOR(l1_data_cache_sc)
        : cache() // 默认构造
    {
        // 使用 SC_CTHREAD，每个时钟上升沿触发
        SC_CTHREAD(run_cycle, clk.pos());
        // rst 信号为复位有效信号（这里假设高有效）
        reset_signal_is(rst, true);
    }

    // 运行周期过程：每个周期调用一次 cache.cycle() 函数
    void run_cycle() {
        unsigned int current_cycle = 0; // 假设 cycle_t 为 unsigned int
        // 复位期间可以进行必要的初始化
        wait(); // 等待第一个时钟周期

        while (true) {
            // 调用 l1_data_cache 中的 cycle() 方法
            cache.cycle(current_cycle);

            // 输出一些调试信息（可选）
            std::cout << "Cycle " << current_cycle << " executed at time " << sc_time_stamp() << std::endl;

            current_cycle++;
            wait(); // 等待下一个时钟上升沿
        }
    }
};

// sc_main() 函数
int sc_main(int argc, char* argv[]) {
    // 创建时钟信号，周期为 10 ns
    sc_clock clk("clk", 10, SC_NS);
    // 复位信号
    sc_signal<bool> rst;

    // 实例化 l1_data_cache_sc 模块
    l1_data_cache_sc cache_inst("cache_inst");
    cache_inst.clk(clk);
    cache_inst.rst(rst);

    // 仿真前给 rst 赋初值（高复位）
    rst.write(true);
    sc_start(1, SC_NS);
    // 释放复位
    rst.write(false);

    // 启动仿真，运行一定时间（例如 1000 ns）
    sc_start(1000, SC_NS);
    return 0;
}
