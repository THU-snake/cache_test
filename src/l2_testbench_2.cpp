#include "DEBUG_L2_model.h"
#include <systemc.h>

SC_MODULE(L2Testbench) {
    //--------------------------
    // 端口和信号定义
    //--------------------------
    sc_clock clk;                          // 时钟信号
    sc_signal<bool> reset;                 // 复位信号

    //--------------------------
    // 待测模块实例
    //--------------------------
    DEBUG_L2_model l2_cache;               // 使用DEBUG_L2_model模块

    //--------------------------
    // 构造函数
    //--------------------------
    SC_CTOR(L2Testbench)
        : clk("clk", 10, SC_NS),          // 100MHz 时钟
          l2_cache(1) {                   // 初始化时传入verbose_level
        // 注册测试进程
        SC_THREAD(run_tests);
        sensitive << clk.posedge_event();

        // 每周期推进L2模型状态
        SC_METHOD(advance_cycle);
        sensitive << clk.posedge_event();
        dont_initialize();
    }

    //--------------------------
    // 周期推进方法
    //--------------------------
    void advance_cycle() {
        l2_cache.cycle(); // 每个时钟周期推进L2模型状态
    }

    //--------------------------
    // 测试主逻辑
    //--------------------------
    void run_tests() {
        // 测试用例 1: 基础读取（缓存命中）
        test_read_hit(0x0); // 地址在L2容量范围内

        // 测试用例 2: 基础写入（缓存缺失）
        test_write_miss(0x10, 0x12345678); // 地址在L2容量范围内

        // 测试用例 3: 并发请求测试
        test_concurrent_access();

        // 结束仿真
        sc_stop();
    }

    //--------------------------
    // 测试用例 1: 读取命中
    //--------------------------
    void test_read_hit(uint32_t addr) {
        std::cout << "===== Test 1: Read Hit =====" << std::endl;

        // 步骤 1: 发送 Get 请求
        dcache_2_L2_memReq req;
        req.a_opcode = Get;
        req.a_param = 0x0;     // 常规读取
        req.a_source = 1;      // 请求源标识
        req.a_address = addr;  // 地址
        req.a_mask.fill(true); // 全字掩码
        req.a_data.fill(0);    // 读取时数据无效

        // 处理请求并推进3个周期
        l2_cache.DEBUG_L2_memReq_process(req, sc_time_stamp().value());
        wait(3 * clk.period());

        // 验证响应
        if (!l2_cache.return_Q_is_empty()) {
            auto rsp = l2_cache.DEBUG_serial_pop();
            if (rsp.d_opcode == AccessAckData) {
                std::cout << "[PASS] Read Hit at 0x" << std::hex << addr 
                          << ", Data: 0x" << rsp.d_data[0] << std::endl;
            }
        } else {
            std::cerr << "[FAIL] No response received!" << std::endl;
        }
    }

    //--------------------------
    // 测试用例 2: 写入缺失
    //--------------------------
    void test_write_miss(uint32_t addr, uint32_t data) {
        std::cout << "===== Test 2: Write Miss =====" << std::endl;

        // 步骤 1: 发送 PutFullData 请求
        dcache_2_L2_memReq req;
        req.a_opcode = PutFullData;
        req.a_param = 0x0;
        req.a_source = 2;
        req.a_address = addr;
        req.a_mask.fill(true);
        req.a_data[0] = data;

        // 处理请求并推进3个周期
        l2_cache.DEBUG_L2_memReq_process(req, sc_time_stamp().value());
        wait(3 * clk.period());

        // 验证响应
        if (!l2_cache.return_Q_is_empty()) {
            auto rsp = l2_cache.DEBUG_serial_pop();
            if (rsp.d_opcode == AccessAck) {
                std::cout << "[PASS] Write Miss at 0x" << std::hex << addr << std::endl;
            }
        } else {
            std::cerr << "[FAIL] No response received!" << std::endl;
        }
    }

    //--------------------------
    // 测试用例 3: 并发请求测试
    //--------------------------
    void test_concurrent_access() {
        std::cout << "===== Test 3: Concurrent Access =====" << std::endl;

        // 发送两个请求
        dcache_2_L2_memReq req1, req2;
        
        // 第一个请求
        req1.a_opcode = Get;
        req1.a_address = 0x20;
        req1.a_source = 3;
        l2_cache.DEBUG_L2_memReq_process(req1, sc_time_stamp().value());
        
        // 第二个请求（立即发送）
        req2.a_opcode = Get;
        req2.a_address = 0x30;
        req2.a_source = 4;
        l2_cache.DEBUG_L2_memReq_process(req2, sc_time_stamp().value());

        // 等待6个周期（3周期/请求 × 2请求）
        wait(6 * clk.period());

        // 验证响应数量
        int resp_count = 0;
        while (!l2_cache.return_Q_is_empty()) {
            l2_cache.DEBUG_serial_pop();
            resp_count++;
        }
        if (resp_count == 2) {
            std::cout << "[PASS] Concurrent Access Test" << std::endl;
        } else {
            std::cerr << "[FAIL] Expected 2 responses, got " << resp_count << std::endl;
        }
    }
};

int sc_main(int argc, char* argv[]) {
    // 创建波形文件
    sc_trace_file* tf = sc_create_vcd_trace_file("l2_cache_wave");

    // 实例化测试平台
    L2Testbench tb("tb");

    // 跟踪时钟信号
    sc_trace(tf, tb.clk, "clk");

    // 启动仿真
    sc_start(200, SC_NS);

    // 关闭波形文件
    sc_close_vcd_trace_file(tf);
    return 0;
}
