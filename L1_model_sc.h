#include <systemc.h>
#include "l1_data_cache.h"

SC_MODULE(L1DataCache) {
    //--------------------------
    // 端口定义
    //--------------------------
    sc_in_clk         clk;          // 时钟信号
    sc_in<bool>       reset;        // 异步复位
    sc_in<LSU_2_dcache_coreReq> core_req;  // 核心请求
    sc_out<dcache_2_LSU_coreRsp> core_rsp; // 核心响应
    sc_out<dcache_2_L2_memReq> mem_req;    // 内存请求
    sc_in<L2_2_dcache_memRsp> mem_rsp;     // 内存响应

    //--------------------------
    // 内部信号
    //--------------------------
    sc_signal<bool> pipeline_stall; // 流水线暂停信号

    //--------------------------
    // 子模块实例
    //--------------------------
    tag_array m_tag_array;
    data_array m_data_array;
    mshr m_mshr;
    wshr m_wshr;

    //--------------------------
    // 构造函数
    //--------------------------
    SC_CTOR(L1DataCache) : 
        m_tag_array("tag_array"),
        m_data_array("data_array"),
        m_mshr("mshr"),
        m_wshr("wshr") 
    {
        // 注册时序进程
        SC_METHOD(process_pipeline);
        sensitive << clk.pos();
        async_reset_signal_is(reset, true);

        // 注册组合逻辑进程
        SC_METHOD(update_outputs);
        sensitive << mem_rsp << core_req;
    }

    //--------------------------
    // 进程：处理流水线逻辑
    //--------------------------
    void process_pipeline() {
        if (reset.read()) {
            // 复位所有内部状态
            m_tag_array.reset();
            m_data_array.reset();
            m_mshr.reset();
            m_wshr.reset();
            return;
        }

        // 流水线阶段调用
        handle_core_request();
        handle_mem_response();
        handle_mem_request();
    }

    //--------------------------
    // 核心请求处理
    //--------------------------
    void handle_core_request() {
        if (core_req.read().is_valid()) {
            // 从端口读取请求
            auto req = core_req.read();
            
            // 调用原有逻辑（示例：Read操作）
            if (req.m_opcode == Read) {
                u_int32_t way_idx;
                auto status = m_tag_array.probe(req.m_block_idx, way_idx);
                
                if (status == HIT) {
                    // 命中处理
                    auto data = m_data_array.read(get_set_idx(req.m_block_idx), way_idx);
                    generate_core_response(req, data);
                } else {
                    // 缺失处理
                    handle_miss(req);
                }
            }
        }
    }

    //--------------------------
    // 内存响应处理
    //--------------------------
    void handle_mem_response() {
        if (mem_rsp.read().is_valid()) {
            auto rsp = mem_rsp.read();
            
            // 更新数据阵列
            m_data_array.write(rsp.d_address, rsp.d_data);
            
            // 更新MSHR状态
            m_mshr.update(rsp.d_source);
        }
    }

    //--------------------------
    // 组合逻辑：更新输出
    //--------------------------
    void update_outputs() {
        // 生成内存请求（示例）
        if (!m_mshr.is_empty()) {
            auto req = m_mshr.generate_mem_request();
            mem_req.write(req);
        }
    }

    //--------------------------
    // 工具方法
    //--------------------------
    void generate_core_response(const LSU_2_dcache_coreReq& req, const cache_line_t& data) {
        dcache_2_LSU_coreRsp rsp;
        // ...填充响应数据...
        core_rsp.write(rsp);
    }

    void handle_miss(const LSU_2_dcache_coreReq& req) {
        // 分配MSHR条目
        m_mshr.allocate(req);
        
        // 生成内存请求
        auto mem_req = create_mem_request(req);
        mem_req.write(mem_req);
    }
};
SC_MODULE(TagArray) {
    sc_in_clk clk;
    sc_in<bool> reset;
    sc_in<u_int32_t> probe_addr;
    sc_out<u_int32_t> hit_way;

    // 原有 C++ 逻辑
    std::vector<tag_entry> entries;

    SC_CTOR(TagArray) {
        SC_METHOD(process_probe);
        sensitive << clk.pos();
    }

    void process_probe() {
        if (reset.read()) { /* 复位逻辑 */ }
        // 实现 tag 查找逻辑
    }
};
SC_MODULE(DataArray) {
    sc_in_clk clk;
    sc_in<u_int32_t> read_addr;
    sc_out<cache_line_t> read_data;
    
    SC_CTOR(DataArray) {
        SC_METHOD(process_read);
        sensitive << clk.pos();
    }
};
SC_MODULE(Top) {
    sc_clock clk;
    sc_signal<bool> reset;
    
    L1DataCache l1_cache;
    DEBUG_L2_model l2_cache;

    SC_CTOR(Top) : clk("clk", 10, SC_NS) {
        // L1 缓存连接
        l1_cache.clk(clk);
        l1_cache.reset(reset);
        
        // L1-L2 接口连接
        l1_cache.mem_req(l2_cache.mem_req);
        l2_cache.mem_rsp(l1_cache.mem_rsp);
    }
};
void trace_signals(sc_trace_file* tf) {
    sc_trace(tf, l1_cache.core_req, "core_req");
    sc_trace(tf, l1_cache.core_rsp, "core_rsp");
    sc_trace(tf, l1_cache.mem_req, "mem_req");
    sc_trace(tf, l1_cache.mem_rsp, "mem_rsp");
}