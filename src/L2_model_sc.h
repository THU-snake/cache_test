#include <systemc.h>
#include "DEBUG_L2_model.h"

SC_MODULE(L2Cache) {
    // 端口定义
    sc_in_clk         clk;          // 时钟
    sc_in<bool>       reset;        // 复位信号
    sc_in<dcache_2_L2_memReq> mem_req; // 内存请求
    sc_out<L2_2_dcache_memRsp> mem_rsp; // 内存响应

    // 内部信号
    sc_signal<bool>   req_valid;    // 请求有效信号
    sc_signal<bool>   rsp_ready;    // 响应就绪信号

    // L2 缓存实例
    DEBUG_L2_model l2_cache;

    SC_CTOR(L2Cache) : l2_cache("l2_cache") {
        // 注册进程
        SC_METHOD(process_mem_req);
        sensitive << clk.pos();
        async_reset_signal_is(reset, true);

        SC_METHOD(process_mem_rsp);
        sensitive << clk.pos();
    }

    // 进程：处理内存请求
    void process_mem_req() {
        if (reset.read()) {
            req_valid.write(false);
            return;
        }

        if (mem_req.read().is_valid()) {
            l2_cache.DEBUG_L2_memReq_process(mem_req.read(), sc_time_stamp().to_double());
            req_valid.write(true);
        } else {
            req_valid.write(false);
        }
    }

    // 进程：生成内存响应
    void process_mem_rsp() {
        if (!l2_cache.return_Q_is_empty()) {
            mem_rsp.write(l2_cache.DEBUG_serial_pop());
            rsp_ready.write(true);
        } else {
            rsp_ready.write(false);
        }
    }
};