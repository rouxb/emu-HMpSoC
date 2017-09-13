/**
 * @file bfs_bulk_hws.cc
 * @brief HW Stub for BFS_BULK from machsuite.
 */
#include "bfs_bulk_hws.h"

/**
 * @brief emu-hmpsoc machsuite bfs_bulk HW_Stub constructor.
 * @params name: systemC entity name.
 * @params compTime_ns: execution Time[ns] of one iteration.
 * @params compTime_ns: execution Energy[nJ] of one iteration.
 */
bfs_bulk_hwStub::bfs_bulk_hwStub(sc_core::sc_module_name name, uint compTime_ns, uint compEn_nj)
  : hwIP(name, (uint8_t)1 /*One HPx*/, (uint8_t)1 /*One irq*/, GENIP_RPSIZE),
    cTime_ns(compTime_ns), cEn_nj(compEn_nj)
{
  //Register THREAD
  SC_THREAD(parse_cmd);
  sensitive << hwIP::cmd_events;
}

bfs_bulk_hwStub::~bfs_bulk_hwStub(){
}

/**
 * @brief emu-hmpsoc SC_Thread implementation for machsuite bfs_bulk HW_Stub .
 */
void bfs_bulk_hwStub::parse_cmd(){
  wait(); // prevent execution at bootup
  while(1) {
#ifdef MACHSUITE_DEBUG
    std::cout << "@ "<< sc_time_stamp() << " ==>"<< "BFS_BULK received cmd." << std::endl;
#endif
    switch (*((uint8_t*)_REG(CMD)))
      {
      case CMD_START:
        irqn[0].write(0); // reset irq Done
        *((uint8_t*)_REG(STATUS)) &= ~(STATUS_DONE + STATUS_IRQ(0));
#ifdef MACHSUITE_DEBUG
        std::cout << "BFS_BULK_HW started for "<< (uint32_t) *((uint32_t*)_REG(PARAMS)) <<" iterations\n";
        std::cout << "ARRAY IN 0x"<<std::hex << (uintptr_t) *((uint32_t*)_REG(ARRAY_IN)) <<"\n";
        std::cout << "ARRAY OUT 0x"<<std::hex << (uintptr_t) *((uint32_t*)_REG(ARRAY_OUT)) <<"\n";
#endif
        for( uint i=0; i< *((uint32_t*)_REG(PARAMS)); i++){
          for(int i=0; i<N_NODES; i++) {lvl[i]=MAX_LEVEL;} // Max-ify levels
          //read input data
          memRead(*((uint32_t*)_REG(ARRAY_IN))
                  + i*(sizeof(node_index_t) + sizeof(node_t)*N_NODES + sizeof(edge_t)*N_EDGES),
                  &starting_node, sizeof(node_index_t), 0);
          memRead(*((uint32_t*)_REG(ARRAY_IN))
                  + i*(sizeof(node_index_t) + sizeof(node_t)*N_NODES + sizeof(edge_t)*N_EDGES)
                  + sizeof(node_index_t),
                  nodes, 2*N_NODES*sizeof(node_t), 0);
          memRead(*((uint32_t*)_REG(ARRAY_IN))
                  + i*(sizeof(node_index_t) + sizeof(node_t)*N_NODES + sizeof(edge_t)*N_EDGES)
                  + (sizeof(node_index_t) + sizeof(node_t)*N_NODES),
                  edges, N_EDGES*sizeof(edge_t), 0);

          bfs(nodes, edges, starting_node, lvl, loc_lvlCnts);
          wait(cTime_ns, SC_NS);
          // FIXME: add Computation Energy monitoring feature
          // FIXME: append energy and time cost to monSystem
          //write output
          memWrite(*((uint32_t*)_REG(ARRAY_OUT)) + i*N_LEVELS*sizeof(edge_index_t),
                   loc_lvlCnts, N_LEVELS*sizeof(edge_index_t), 0);
        }
        irqn[0].write(1); // set irq Done
        *((uint8_t*)_REG(STATUS)) = STATUS_DONE + STATUS_IRQ(0);
        *((uint8_t*)_REG(CMD)) = 0x00;
        break;;

      case CMD_RST_IRQ(0):
        irqn[0].write(0); // reset irq Done
        *((uint8_t*)_REG(STATUS)) &= ~(STATUS_DONE + STATUS_IRQ(0));
        *((uint8_t*)_REG(CMD)) = 0x00;
        break;;
      default:
        *((uint8_t*)_REG(CMD)) = 0x00;
        break;;
      }
    wait();
  }
}
