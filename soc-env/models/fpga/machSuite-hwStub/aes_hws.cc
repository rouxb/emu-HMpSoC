/**
 * @file aes_hws.cc
 * @brief HW Stub for AES from machsuite.
 */
#include "aes_hws.h"

/**
 * @brief emu-hmpsoc machsuite aes HW_Stub constructor.
 * @params name: systemC entity name.
 * @params compTime_ns: execution Time[ns] of one iteration.
 * @params compTime_ns: execution Energy[nJ] of one iteration.
 */
aes_hwStub::aes_hwStub(sc_core::sc_module_name name, uint compTime_ns, uint compEn_nj)
  : hwIP(name, (uint8_t)1 /*One HPx*/, (uint8_t)1 /*One irq*/, GENIP_RPSIZE),
    cTime_ns(compTime_ns), cEn_nj(compEn_nj)
{
  //Register THREAD
  SC_THREAD(parse_cmd);
  sensitive << hwIP::cmd_events;
}

aes_hwStub::~aes_hwStub(){
}

/**
 * @brief emu-hmpsoc SC_Thread implementation for machsuite aes HW_Stub .
 */
void aes_hwStub::parse_cmd(){
  wait(); // prevent execution at bootup
  while(1) {
#ifdef MACHSUITE_DEBUG
    std::cout << "@ "<< sc_time_stamp() << " ==>"<< "AES received cmd: "
              <<(uint)(*((uint8_t*)_REG(CMD)))<< std::endl;
#endif
    switch (*((uint8_t*)_REG(CMD)))
      {
      case CMD_START:
        irqn[0].write(0); // reset irq Done
        *((uint8_t*)_REG(STATUS)) &= ~(STATUS_DONE + STATUS_IRQ(0));

#ifdef MACHSUITE_DEBUG
        std::cout << "AES_HW started for "<< (uint32_t) *((uint32_t*)_REG(PARAMS)) <<" iterations\n";
        std::cout << "ARRAY IN 0x"<<std::hex << (uintptr_t) *((uint32_t*)_REG(ARRAY_IN)) <<"\n";
        std::cout << "ARRAY OUT 0x"<<std::hex << (uintptr_t) *((uint32_t*)_REG(ARRAY_OUT)) <<"\n";
#endif
        //read key
        memRead(*((uint32_t*)_REG(ARRAY_IN)), key, 32*sizeof(uint8_t), 0);
        for( int i=0; i< *((uint32_t*)_REG(PARAMS)); i++){
          //read input data
          memRead(*((uint32_t*)_REG(ARRAY_IN)) +(32+ i*16)*sizeof(uint8_t)
                  , buf, 16*sizeof(uint8_t), 0);
          aes256_encrypt_ecb(&ctx, key, buf);
          wait(cTime_ns, SC_NS);
          // FIXME: add Computation Energy monitoring feature
          // FIXME: append energy and time cost to monSystem
          //write output
          memWrite(*((uint32_t*)_REG(ARRAY_OUT)) +(i*16)*sizeof(uint8_t)
                  , buf, 16*sizeof(uint8_t), 0);
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
#ifdef MACHSUITE_DEBUG
        std::cout << "@ "<< sc_time_stamp() << " ==>"<< "AES cmd fell in default."
                  << (*((uint8_t*)_REG(CMD)))<< std::endl;
#endif
        *((uint8_t*)_REG(CMD)) = 0x00;
        break;;
      }
    wait();
  }
}
