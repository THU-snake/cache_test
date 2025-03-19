#include "L2_model_sc.h"
#include <systemc.h>

SC_MODULE(L2Testbench) {
    //--------------------------
    // 端口和信号定义
    //--------------------------
    sc_clock clk;                          // 时钟信号
    sc_signal<bool> reset;                 // 复位信号
    sc_signal<dcache_2_L2_memReq> mem_req; // L1 -> L2 的请求
    sc_signal<L2_2_dcache_memRsp> mem_rsp; // L2 -> L1 的响应

    //--------------------------
    // 待测模块实例
    //--------------------------
    DEBUG_L2_model l2_cache;

    //--------------------------
    // 构造函数
    //--------------------------
    SC_CTOR(L2Testbench)
        : clk("clk", 10, SC_NS)
        , // 100MHz 时钟
        l2_cache("l2_cache") {
        // 连接端口
        l2_cache.clk(clk);
        l2_cache.reset(reset);
        l2_cache.mem_req(mem_req);
        l2_cache.mem_rsp(mem_rsp);

        // 注册测试进程
        SC_THREAD(run_tests);
        sensitive << clk.pos();
    }

    //--------------------------
    // 测试主逻辑
    //--------------------------
    void run_tests() {
        // 测试用例 1: 基础读取（缓存命中）
        test_read_hit(0x1000);

        // 测试用例 2: 基础写入（缓存缺失）
        test_write_miss(0x2000, 0x12345678);

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

        // 步骤 1: 复位
        reset.write(true);
        wait(2, SC_NS);
        reset.write(false);
        wait(10, SC_NS);

        // 步骤 2: 发送 Get 请求
        dcache_2_L2_memReq req;
        req.a_opcode = Get;
        req.a_param = 0x0;     // 常规读取
        req.a_source = 1;      // 请求源标识
        req.a_address = addr;  // 地址 0x1000
        req.a_mask.fill(true); // 全字掩码
        req.a_data.fill(0);    // 读取时数据无效

        mem_req.write(req);
        wait(clk.posedge_event());

        // 步骤 3: 等待响应（假设 L2 延迟为 3 周期）
        wait(3 * clk.period());

        // 步骤 4: 验证响应
        if (mem_rsp.read().d_opcode == AccessAckData) {
            std::cout << "[PASS] Read Hit at 0x" << std::hex << addr << ", Data: 0x" << mem_rsp.read().d_data[0]
                      << std::endl;
        } else {
            std::cerr << "[FAIL] Read Hit Test Failed!" << std::endl;
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
        req.a_data[0] = data; // 写入数据 0x12345678

        mem_req.write(req);
        wait(clk.posedge_event());

        // 步骤 2: 等待响应（假设 L2 延迟为 3 周期）
        wait(3 * clk.period());

        // 步骤 3: 验证响应
        if (mem_rsp.read().d_opcode == AccessAck) {
            std::cout << "[PASS] Write Miss at 0x" << std::hex << addr << std::endl;
        } else {
            std::cerr << "[FAIL] Write Miss Test Failed!" << std::endl;
        }
    }

    //--------------------------
    // 测试用例 3: 并发请求测试
    //--------------------------
    void test_concurrent_access() {
        std::cout << "===== Test 3: Concurrent Access =====" << std::endl;

        // 步骤 1: 同时发送两个请求（测试仲裁逻辑）
        dcache_2_L2_memReq req1, req2;
        req1.a_opcode = Get;
        req1.a_address = 0x3000;
        req2.a_opcode = Get;
        req2.a_address = 0x4000;

        // 先发送第一个请求
        mem_req.write(req1);
        wait(clk.posedge_event());

        // 在未完成时发送第二个请求（测试是否会阻塞）
        mem_req.write(req2);
        wait(clk.posedge_event());

        // 等待两个响应
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

    // 跟踪关键信号
    sc_trace(tf, tb.clk, "clk");
    sc_trace(tf, tb.reset, "reset");
    sc_trace(tf, tb.mem_req, "mem_req");
    sc_trace(tf, tb.mem_rsp, "mem_rsp");

    // 启动仿真
    sc_start(200, SC_NS);

    // 关闭波形文件
    sc_close_vcd_trace_file(tf);
    return 0;
}