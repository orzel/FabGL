/*
  Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com) - <http://www.fabgl.com>
  Copyright (c) 2019-2022 Fabrizio Di Vittorio.
  All rights reserved.


* Please contact fdivitto2013@gmail.com if you need a commercial license.


* This library and related software is available under GPL v3.

  FabGL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  FabGL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with FabGL.  If not, see <http://www.gnu.org/licenses/>.
 */



#include <alloca.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "soc/i2s_struct.h"
#include "soc/i2s_reg.h"
#include "driver/periph_ctrl.h"
#include "soc/rtc.h"

#include "fabutils.h"
#include "swgenerator.h"
#include "soc/rtc_io_reg.h"
#include "soc/gpio_sig_map.h"
#include "rom/gpio.h"

#pragma GCC optimize ("O2")


namespace fabgl {



/*************************************************************************************/
/* GPIOStream definitions */

void GPIOStream::begin()
{
  m_DMAStarted = false;
  m_DMABuffer  = nullptr;
  m_DMAData    = nullptr;
}

void GPIOStream::end()
{
  stop();
}



// if bit is -1 = clock signal
// gpio = GPIO_UNUSED means not set
void GPIOStream::setupGPIO(gpio_num_t gpio, int bit, gpio_mode_t mode)
{
  if (gpio != GPIO_UNUSED) {

    if (bit == -1) {
      // I2S1 clock out to CLK_OUT1 (fixed to GPIO0)
      WRITE_PERI_REG(PIN_CTRL, 0xF);
      PIN_FUNC_SELECT(GPIO_PIN_REG_0, FUNC_GPIO0_CLK_OUT1);
    } else {
      configureGPIO(gpio, mode);
      gpio_matrix_out(gpio, I2S1O_DATA_OUT0_IDX + bit, false, false);
    }
    
  }
}


void GPIOStream::play(int freq, lldesc_t volatile * dmaBuffers)
{
  if (!m_DMAStarted) {

    // Power on device
    periph_module_enable(PERIPH_I2S1_MODULE);

    // Initialize I2S device
    I2S1.conf.tx_reset = 1;
    I2S1.conf.tx_reset = 0;

    // Reset DMA
    I2S1.lc_conf.out_rst = 1;
    I2S1.lc_conf.out_rst = 0;

    // Reset FIFO
    I2S1.conf.tx_fifo_reset = 1;
    I2S1.conf.tx_fifo_reset = 0;

    // LCD mode
    I2S1.conf2.val            = 0;
    I2S1.conf2.lcd_en         = 1;
    I2S1.conf2.lcd_tx_wrx2_en = 1;
    I2S1.conf2.lcd_tx_sdx2_en = 0;

    I2S1.sample_rate_conf.val         = 0;
    I2S1.sample_rate_conf.tx_bits_mod = 8;

    setupClock(freq);

    I2S1.fifo_conf.val                  = 0;
    I2S1.fifo_conf.tx_fifo_mod_force_en = 1;
    I2S1.fifo_conf.tx_fifo_mod          = 1;
    I2S1.fifo_conf.tx_fifo_mod          = 1;
    I2S1.fifo_conf.tx_data_num          = 32;
    I2S1.fifo_conf.dscr_en              = 1;

    I2S1.conf1.val           = 0;
    I2S1.conf1.tx_stop_en    = 0;
    I2S1.conf1.tx_pcm_bypass = 1;

    I2S1.conf_chan.val         = 0;
    I2S1.conf_chan.tx_chan_mod = 1;

    I2S1.conf.tx_right_first = 1;

    I2S1.timing.val = 0;

    // Reset AHB interface of DMA
    I2S1.lc_conf.ahbm_rst      = 1;
    I2S1.lc_conf.ahbm_fifo_rst = 1;
    I2S1.lc_conf.ahbm_rst      = 0;
    I2S1.lc_conf.ahbm_fifo_rst = 0;

    // Start DMA
    I2S1.lc_conf.val    = I2S_OUT_DATA_BURST_EN | I2S_OUTDSCR_BURST_EN;
    I2S1.out_link.addr  = (uint32_t) (dmaBuffers ? &dmaBuffers[0] : m_DMABuffer);
    I2S1.out_link.start = 1;
    I2S1.conf.tx_start  = 1;

    m_DMAStarted = true;

  }
}


void GPIOStream::stop()
{
  if (m_DMAStarted) {

    rtc_clk_apll_enable(false);
    periph_module_disable(PERIPH_I2S1_MODULE);

    m_DMAStarted = false;
  }
}


void GPIOStream::setupClock(int freq)
{
  APLLParams p = {0, 0, 0, 0};
  double error, out_freq;
  uint8_t a = 1, b = 0;
  APLLCalcParams(freq, &p, &a, &b, &out_freq, &error);

  I2S1.clkm_conf.val          = 0;
  I2S1.clkm_conf.clkm_div_b   = b;
  I2S1.clkm_conf.clkm_div_a   = a;
  I2S1.clkm_conf.clkm_div_num = 2;  // not less than 2
  
  I2S1.sample_rate_conf.tx_bck_div_num = 1; // this makes I2S1O_BCK = I2S1_CLK

  rtc_clk_apll_enable(true);
  rtc_clk_apll_coeff_set(p.o_div, p.sdm0, p.sdm1, p.sdm2);
  I2S1.clkm_conf.clka_en = 1;
}


} // end of namespace


