#include "lwip/opt.h"

#include "lwip/sys.h"
#include "lwip/api.h"

#include "stm32f4xx.h"
#include "stm32f4x7_eth_bsp.h"
#include <stdbool.h>

#define MESSAGE_SIZE 1
extern bool drive_state;

void app_rcv_dataTask(void *pvParameters)
{
  struct netconn *conn = netconn_new(NETCONN_UDP);
  struct netbuf *buf;
  
  if (conn!= NULL) {
    err_t err = netconn_bind(conn, IP_ADDR_ANY, 7);
    if (err == ERR_OK) {
      while (1) {
        buf = netconn_recv(conn); 
      
        if (buf != NULL) {
          char data[MESSAGE_SIZE];        
          netbuf_copy(buf, data, MESSAGE_SIZE);
          
          drive_state = data[0];
          printf("Drive state: %i\n", drive_state);
          netbuf_delete(buf);
        }
        vTaskDelay(30);
      }
      netconn_delete(conn);
    }
  }

}