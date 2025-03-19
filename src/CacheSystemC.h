#include <systemc.h>
#include "l1_data_cache.h"
#include "DEBUG_L2_model.h"

SC_MODULE(CacheSystemC) {
    // 端口定义
    sc_in_clk         clk;          // 时钟
    sc_in<bool>       reset;        // 复位信号
    sc_in<sc_uint<32>> core_req;    // 核心请求（指令）
    sc_out<bool>      core_rsp;     // 核心响应
    sc_out<sc_uint<32>> data_out;   // 数据输出
    sc_in<sc_uint<32>> data_in;     // 数据输入（用于写操作）

    // 内部信号
    sc_signal<bool>   cache_hit;    // 缓存命中信号
    sc_signal<sc_uint<32>> mem_req; // 内存请求

    // 子模块
    l1_data_cache     dcache;       // L1 数据缓存
    DEBUG_L2_model    L2;           // L2 缓存

    // 构造函数
    SC_CTOR(CacheSystemC) : dcache("dcache"), L2("L2") {
        // 注册进程
        SC_METHOD(process_core_req);
        sensitive << clk.pos();
        async_reset_signal_is(reset, true);

        SC_METHOD(process_mem_rsp);
        sensitive << clk.pos();
    }

    // 进程：处理核心请求
    void process_core_req() {
        if (reset.read()) {
            // 复位逻辑
            cache_hit.write(false);
            core_rsp.write(false);
            return;
        }

        // 解析核心请求
        LSU_2_dcache_coreReq coreReq;
        if (parse_instruction(core_req.read(), coreReq, sc_time_stamp().to_double())) {
            dcache.process_core_req(coreReq);
            cache_hit.write(dcache.check_hit(coreReq));
        }
    }

    // 进程：处理内存响应
    void process_mem_rsp() {
        if (!L2.return_Q_is_empty()) {
            dcache.process_mem_rsp(L2.DEBUG_serial_pop());
        }
    }

    // 解析指令（从原代码中移植）
    bool parse_instruction(sc_uint<32> instruction, LSU_2_dcache_coreReq& coreReq, double time) {
        // 移植原代码中的 parse_instruction 逻辑
        // ...
    }
};
void trace_signals(sc_trace_file* tf, CacheSystemC& cache) {
    sc_trace(tf, cache.clk, "clk");
    sc_trace(tf, cache.reset, "reset");
    sc_trace(tf, cache.core_req, "core_req");
    sc_trace(tf, cache.core_rsp, "core_rsp");
    sc_trace(tf, cache.data_out, "data_out");
    sc_trace(tf, cache.cache_hit, "cache_hit");
}
int sc_main(int argc, char* argv[]) {
    sc_trace_file* tf = sc_create_vcd_trace_file("cache_waveform");
    CacheSystemC cache("cache");
    trace_signals(tf, cache);

    // 仿真运行
    sc_start(100, SC_NS);

    sc_close_vcd_trace_file(tf);
    return 0;
}