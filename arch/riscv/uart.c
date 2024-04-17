#include <uart.h>
#include <string.h>
#include <spinlock.h>

#define REG(r)                  ((volatile unsigned char *)(UART0 + r))

/* receive holding register (for input bytes) */
#define RHR                     0          
// transmit holding register (for output bytes)       
#define THR                     0
/* interrupt enable register */             
#define IER                     1                 
#define IER_RX_ENABLE           (1<<0)
#define IER_TX_ENABLE           (1<<1)
/* FIFO control register */
#define FCR                     2                 
#define FCR_FIFO_ENABLE         (1<<0)
/* clear the content of the two FIFOs */
#define FCR_FIFO_CLEAR          (3<<1) 
/* interrupt status register */
#define ISR                     2        
/* line control register */         
#define LCR                     3                 
#define LCR_EIGHT_BITS          (3<<0)
/* special mode to set baud rate */
#define LCR_BAUD_LATCH          (1<<7) 
/* line status register */
#define LSR                     5    
/* input is waiting to be read from RHR */             
#define LSR_RX_READY            (1<<0)   
/* THR can accept another character to send */
#define LSR_TX_IDLE             (1<<5)    

#define REG_R(r)                (*(REG(r)))
#define REG_W(r, v)             (*(REG(r)) = (v))

#define UART_TX_BUF_SIZE 32

extern volatile uint8 paniced;

static struct spinlock spinlock_uart;
char uart_tx_buf[UART_TX_BUF_SIZE];
uint64 uart_tx_w; // write next to uart_tx_buf[uart_tx_w % UART_TX_BUF_SIZE]
uint64 uart_tx_r; // read next from uart_tx_buf[uart_tx_r % UART_TX_BUF_SIZE]

void uart_init(void)
{
        /* disable interrupts. */
        REG_W(IER, 0x00);

        /* special mode to set baud rate. */
        REG_W(LCR, LCR_BAUD_LATCH);

        /* LSB for baud rate of 38.4K. */
        REG_W(0, 0x03);

        /* MSB for baud rate of 38.4K. */
        REG_W(1, 0x00);

        /* 
                leave set-baud mode,
                and set word length to 8 bits, no parity. 
        */
        REG_W(LCR, LCR_EIGHT_BITS);

        /* reset and enable FIFOs. */
        REG_W(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);

        /* enable transmit and receive interrupts. */
        // REG_W(IER, IER_TX_ENABLE | IER_RX_ENABLE);

        spinlock_init(&spinlock_uart, "uart");
}

void uart_start(void)
{
        if(paniced) {
                for(;;);        
        }
        while(1){
                if(uart_tx_w == uart_tx_r){
                        /* Transmit buffer is empty. */
                        return;
                }

                if((REG_R(LSR) & LSR_TX_IDLE) == 0){
                        /*  
                                the UART transmit holding register is full,
                                so we cannot give it another byte.
                                it will interrupt when it's ready for a new byte. 
                        */
                        return;
                }

                int c = uart_tx_buf[uart_tx_r % UART_TX_BUF_SIZE];
                uart_tx_r += 1;

                /* TODO: We should wakeup sleeping thread here */
                /* wakeup() */

                REG_W(THR, c);
        }  
}

/* 
        IMPORTANCE: 
        This function can't be called in the interrupt!
        Because it may causes blocked.
 */
void uart_putc(int c)
{
        spinlock_acquire(&spinlock_uart);

        if(paniced) {
                for(;;);        
        }

        while(uart_tx_w == uart_tx_r + UART_TX_BUF_SIZE){
                /* Buffer is full. Wait for uart_start() to open up space in the buffer. */
                /* TODO: We should add a function 'sleep' */
                /* sleep() */
        }
        uart_tx_buf[uart_tx_w % UART_TX_BUF_SIZE] = c;
        uart_tx_w += 1;
        uart_start();
        spinlock_release(&spinlock_uart);
}

/* 
        Alternate version of uart_putc() 
        It can be called in the interrupt
*/
void uart_putc_sync(int c)
{
      enter_critical();

      if(paniced) {
                for(;;);        
      }

      while((REG_R(LSR) & LSR_TX_IDLE) == 0);
      REG_W(THR, c);

      exit_critical();  
}

void uart_puts(const char* str)
{
        int n;
        n = strlen(str);

        while(n --) {
                uart_putc_sync(*str++);
        }
}

int uart_getc(void)
{
        if(REG_R(LSR) & 0x01) {
                return REG_R(RHR);
        } else {
                return -1;
        }
}