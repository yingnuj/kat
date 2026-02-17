#include <hardware/gpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"

#define SPI_DMA_BUF_SIZE 1024 * 16
#define SPI0_INSTANCE spi0
#define SPI_RX_PIN 16
#define SPI_CSN_PIN 17
#define SPI_SCK_PIN 18
#define SPI_TX_PIN 19
#define SPI_BAUD_RATE (1000 * 1000 * 1)
#define SLAVE_SYNC_PIN 10
#define MASTER_SYNC_PIN 11

static uint dma_tx;
static uint dma_rx;

static uint8_t txbuf[SPI_DMA_BUF_SIZE];
static uint8_t rxbuf[SPI_DMA_BUF_SIZE];

static void hex_dump(const uint8_t *buf, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    printf("%02x ", buf[i]);
    if ((i + 1) % 16 == 0) {
      printf("\n");
    }
  }
  printf("\n");
}

static void fill_tx_buffer() {
  memset(txbuf, 0, SPI_DMA_BUF_SIZE);
  const char kAT_TEST[] = "AT+TEST\r\n";
  uint16_t str_len = strlen(kAT_TEST);
  txbuf[0] = 'A';
  txbuf[1] = 'T';
  txbuf[3] = str_len & 0xFF;
  txbuf[4] = (str_len >> 8) & 0xFF;
  memcpy(&txbuf[5], kAT_TEST, str_len);
}

static void gpio_irq_handler(uint gpio, uint32_t events) {
  // Start the DMA transfers when the slave sync pin goes high
  if (gpio == SLAVE_SYNC_PIN && (events & GPIO_IRQ_EDGE_RISE)) {
    printf("Slave sync pin went high, starting DMA transfers\n");
    dma_start_channel_mask((1u << dma_tx) | (1u << dma_rx));
  }
}

static void tx_dma_irq_handler() {
  // Clear the interrupt request.
  dma_hw->ints0 = 1u << dma_tx;
  printf("TX DMA transfer complete\n");
}

static void rx_dma_irq_handler() {
  // Clear the interrupt request.
  dma_hw->ints1 = 1u << dma_rx;
  printf("RX DMA transfer complete\n");
  printf("Received data:\n");
  hex_dump(rxbuf, SPI_DMA_BUF_SIZE);
}

void board_init() {
  printf("board_init()\n");

  // MASTER_SYNC_PIN init.
  gpio_init(MASTER_SYNC_PIN);
  gpio_set_dir(MASTER_SYNC_PIN, true);
  gpio_pull_down(MASTER_SYNC_PIN);
  gpio_put(MASTER_SYNC_PIN, 1);

  // SLAVE_SYNC_PIN init.
  gpio_init(SLAVE_SYNC_PIN);
  gpio_set_dir(SLAVE_SYNC_PIN, false);
  gpio_pull_down(SLAVE_SYNC_PIN);

  // SPI init.
  spi_init(SPI0_INSTANCE, SPI_BAUD_RATE);
  gpio_set_function(SPI_RX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SPI_SCK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SPI_TX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SPI_CSN_PIN, GPIO_FUNC_SPI);
}

void driver_init() {
  printf("driver_init()\n");
  // SLAVE_SYNC_PIN IRQ init.
  gpio_set_irq_enabled_with_callback(SLAVE_SYNC_PIN, GPIO_IRQ_EDGE_RISE, true,
                                     &gpio_irq_handler);

  // SPI DMA init.
  dma_tx = dma_claim_unused_channel(true);
  dma_rx = dma_claim_unused_channel(true);

  // Configure TX DMA
  dma_channel_config c = dma_channel_get_default_config(dma_tx);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
  channel_config_set_dreq(&c, spi_get_dreq(SPI0_INSTANCE, true));
  channel_config_set_read_increment(&c, true);
  channel_config_set_write_increment(&c, false);
  dma_channel_configure(dma_tx, &c, &spi_get_hw(SPI0_INSTANCE)->dr, txbuf,
                        SPI_DMA_BUF_SIZE, false);

  // Configure RX DMA
  c = dma_channel_get_default_config(dma_rx);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
  channel_config_set_dreq(&c, spi_get_dreq(SPI0_INSTANCE, false));
  channel_config_set_read_increment(&c, false);
  channel_config_set_write_increment(&c, true);
  dma_channel_configure(dma_rx, &c, rxbuf, &spi_get_hw(SPI0_INSTANCE)->dr,
                        SPI_DMA_BUF_SIZE, false);

  dma_channel_set_irq0_enabled(dma_tx, true);
  irq_set_exclusive_handler(DMA_IRQ_0, tx_dma_irq_handler);
  irq_set_enabled(DMA_IRQ_0, true);

  dma_channel_set_irq1_enabled(dma_rx, true);
  irq_set_exclusive_handler(DMA_IRQ_1, rx_dma_irq_handler);
  irq_set_enabled(DMA_IRQ_1, true);

  fill_tx_buffer();
}

int main() {
  stdio_init_all();
  sleep_ms(1000);

  board_init();
  driver_init();

  while (true) {
    tight_loop_contents();
  }

  return 0;
}
