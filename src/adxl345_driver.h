/*
 * adxl345_driver.h
 *
 * See http://www.analog.com/media/en/technical-documentation/data-sheets/ADXL345.PDF
 *
 *  Created on: Jan 2, 2016
 *      Author: petera
 */

#ifndef ADXL345_DRIVER_H_
#define ADXL345_DRIVER_H_

#include "system.h"
#include "i2c_dev.h"

#define I2C_ERR_ADXL345_BUSY     -1500
#define I2C_ERR_ADXL345_ID       -1501
#define I2C_ERR_ADXL345_STATE    -1502
#define I2C_ERR_ADXL345_NULLPTR  -1503

#define ADXL345_R_DEVID           0x00
#define ADXL345_R_THRESH_TAP      0x1d
#define ADXL345_R_OFSX            0x1e
#define ADXL345_R_OFSY            0x1f
#define ADXL345_R_OFSZ            0x20
#define ADXL345_R_DUR             0x21
#define ADXL345_R_LATENT          0x22
#define ADXL345_R_WINDOW          0x23
#define ADXL345_R_THRESH_ACT      0x24
#define ADXL345_R_THRESH_INACT    0x25
#define ADXL345_R_TIME_INACT      0x26
#define ADXL345_R_ACT_INACT_CTL   0x27
#define ADXL345_R_THRESH_FF       0x28
#define ADXL345_R_TIME_FF         0x29
#define ADXL345_R_TAP_AXES        0x2a
#define ADXL345_R_ACT_TAP_STATUS  0x2b
#define ADXL345_R_BW_RATE         0x2c
#define ADXL345_R_POWER_CTL       0x2d
#define ADXL345_R_INT_ENABLE      0x2e
#define ADXL345_R_INT_MAP         0x2f
#define ADXL345_R_INT_SOURCE      0x30
#define ADXL345_R_DATA_FORMAT     0x31
#define ADXL345_R_DATAX0          0x32
#define ADXL345_R_DATAX1          0x33
#define ADXL345_R_DATAY0          0x34
#define ADXL345_R_DATAY1          0x35
#define ADXL345_R_DATAZ0          0x36
#define ADXL345_R_DATAZ1          0x37
#define ADXL345_R_FIFO_CTL        0x38
#define ADXL345_R_FIFO_STATUS     0x39

#define ADXL345_ID                0xe5

#define ADXL345_I2C_ADDR          0xa6

typedef struct {
  s16_t x,y,z;
} adxl_reading;

typedef enum {
  ADXL345_STATE_IDLE = 0,
  ADXL345_STATE_READ,
  ADXL345_STATE_READ_FIFO,
  ADXL345_STATE_READ_INTERRUPTS,
  ADXL345_STATE_READ_ACT_TAP,
  ADXL345_STATE_READ_ALL_STATUS,
  ADXL345_STATE_ID,
  ADXL345_STATE_CONFIG_POWER,
  ADXL345_STATE_SET_OFFSET,
  ADXL345_STATE_CONFIG_TAP,
  ADXL345_STATE_CONFIG_ACTIVITY,
  ADXL345_STATE_CONFIG_FREEFALL,
  ADXL345_STATE_CONFIG_INTERRUPTS,
  ADXL345_STATE_CONFIG_FORMAT,
  ADXL345_STATE_CONFIG_FIFO,
} adxl_state;

typedef enum {
  ADXL345_NONE = 0b000,
  ADXL345_X    = 0b001,
  ADXL345_Y    = 0b010,
  ADXL345_XY   = 0b011,
  ADXL345_Z    = 0b100,
  ADXL345_XZ   = 0b101,
  ADXL345_YZ   = 0b110,
  ADXL345_XYZ  = 0b111,
} adxl_axes;

typedef enum {
  ADXL345_DC    = 0b0,
  ADXL345_AC    = 0b1
} adxl_acdc;

typedef enum {
  ADXL345_RATE_0_10 = 0b0000,
  ADXL345_RATE_0_20 = 0b0001,
  ADXL345_RATE_0_39 = 0b0010,
  ADXL345_RATE_0_78 = 0b0011,
  ADXL345_RATE_1_56 = 0b0100,
  ADXL345_RATE_3_13 = 0b0101,
  ADXL345_RATE_6_25 = 0b0110,
  ADXL345_RATE_12_5_LP = 0b0111,
  ADXL345_RATE_25_LP = 0b1000,
  ADXL345_RATE_50_LP = 0b1001,
  ADXL345_RATE_100_LP = 0b1010,
  ADXL345_RATE_200_LP = 0b1011,
  ADXL345_RATE_400_LP = 0b1100,
  ADXL345_RATE_800 = 0b1101,
  ADXL345_RATE_1600 = 0b1110,
  ADXL345_RATE_3200 = 0b1111,
} adxl_rate;

typedef enum {
  ADXL345_MODE_STANDBY = 0b0,
  ADXL345_MODE_MEASURE = 0b1
} adxl_mode;

typedef enum {
  ADXL345_SLEEP_OFF = 0b000,
  ADXL345_SLEEP_RATE_1 = 0b111,
  ADXL345_SLEEP_RATE_2 = 0b110,
  ADXL345_SLEEP_RATE_4 = 0b101,
  ADXL345_SLEEP_RATE_8 = 0b100,
} adxl_sleep;

typedef enum {
  ADXL345_RANGE_2G = 0b00,
  ADXL345_RANGE_4G = 0b01,
  ADXL345_RANGE_8G = 0b10,
  ADXL345_RANGE_16G = 0b11,
} adxl_range;

typedef enum {
  ADXL345_FIFO_BYPASS = 0b00,
  ADXL345_FIFO_ENABLE = 0b01,
  ADXL345_FIFO_STREAM = 0b10,
  ADXL345_FIFO_TRIGGER = 0b11,
} adxl_fifo_mode;

typedef enum {
  ADXL345_PIN_INT1 = 0b0,
  ADXL345_PIN_INT2 = 0b1,
} adxl_pin_int;

typedef struct {
  union {
    __attribute (( packed )) struct {
      u8_t entries : 6;
      u8_t _rsv : 1;
      u8_t fifo_trig : 1;
    };
    u8_t raw;
  };
} adxl_fifo_status;

typedef struct {
  union {
    __attribute (( packed )) struct {
      u8_t tap_z : 1;
      u8_t tap_y : 1;
      u8_t tap_x : 1;
      u8_t asleep : 1;
      u8_t act_z : 1;
      u8_t act_y : 1;
      u8_t act_x : 1;
      u8_t _rsv : 1;
    };
    u8_t raw;
  };
} adxl_act_tap_status;

typedef struct {
  u8_t int_src;
  adxl_fifo_status fifo_status;
  adxl_act_tap_status act_tap_status;
} __attribute (( packed )) adxl_status;


#define ADXL345_INT_DATA_READY    0b10000000
#define ADXL345_INT_SINGLE_TAP    0b01000000
#define ADXL345_INT_DOUBLE_TAP    0b00100000
#define ADXL345_INT_ACTIVITY      0b00010000
#define ADXL345_INT_INACTIVITY    0b00001000
#define ADXL345_INT_FREE_FALL     0b00000100
#define ADXL345_INT_WATERMARK     0b00000010
#define ADXL345_INT_OVERRUN       0b00000001

typedef struct adxl345_dev_s {
  i2c_dev i2c_dev;
  adxl_state state;
  void (* callback)(struct adxl345_dev_s *dev, adxl_state state, int res);
  i2c_dev_sequence seq[6];
  union {
    bool *id_ok;
    adxl_reading *data;
    adxl_fifo_status *fifo_status;
    adxl_act_tap_status *act_tap_status;
    adxl_status *status;
    u8_t *int_src;
  } arg;
  u8_t tmp_buf[8];
} adxl345_dev;

void adxl_open(adxl345_dev *dev, i2c_bus *bus, u32_t clock, void (*adxl_callback)(adxl345_dev *dev, adxl_state state, int res));
void adxl_close(adxl345_dev *dev);
int adxl_check_id(adxl345_dev *dev, bool *id_ok);

/**
Configures power related configurations.
@param dev        The adxl345 device struct.
@param low_power  A setting of 0 in the LOW_POWER bit selects normal operation,
                  and a setting of 1 selects reduced power operation, which has
                  somewhat higher noise.
@param rate       These bits select the device bandwidth and output data rate. The default
                  value is 0x0A, which translates to a 100 Hz output data rate. An output
                  data rate should be selected that is appropriate for the communication
                  protocol and frequency selected. Selecting too high of an output data
                  rate with a low communication speed results in samples being discarded.
@param link       A setting of 1 in the link bit with both the activity and inactivity
                  functions enabled delays the start of the activity function until
                  inactivity is detected. After activity is detected, inactivity detection
                  begins, preventing the detection of activity. This bit serially links
                  the activity and inactivity functions. When this bit is set to 0,
                  the inactivity and activity functions are concurrent.
                  When clearing the link bit, it is recommended that the part be
                  placed into standby mode and then set back to measurement
                  mode with a subsequent write. This is done to ensure that the
                  device is properly biased if sleep mode is manually disabled;
                  otherwise, the first few samples of data after the link bit is cleared
                  may have additional noise, especially if the device was asleep
                  when the bit was cleared.
@param auto_sleep If the link bit is set, a setting of 1 in the AUTO_SLEEP bit enables
                  the auto-sleep functionality. In this mode, the ADXL345 automatically
                  switches to sleep mode if the inactivity function is
                  enabled and inactivity is detected (that is, when acceleration is
                  below the THRESH_INACT value for at least the time indicated
                  by TIME_INACT). If activity is also enabled, the ADXL345
                  automatically wakes up from sleep after detecting activity and
                  returns to operation at the output data rate set in the BW_RATE
                  register. A setting of 0 in the AUTO_SLEEP bit disables automatic
                  switching to sleep mode.
                  If the link bit is not set, the AUTO_SLEEP feature is disabled
                  and setting the AUTO_SLEEP bit does not have an impact on
                  device operation. Refer to the Link Bit section or the Link Mode
                  section for more information on utilization of the link feature
                  in the datasheet.
                  When clearing the AUTO_SLEEP bit, it is recommended that the
                  part be placed into standby mode and then set back to measurement
                  mode with a subsequent write. This is done to ensure that
                  the device is properly biased if sleep mode is manually disabled;
                  otherwise, the first few samples of data after the AUTO_SLEEP
                  bit is cleared may have additional noise, especially if the device
                  was asleep when the bit was cleared.
@param mode       A setting of 0 in the measure bit places the part into standby mode,
                  and a setting of 1 places the part into measurement mode. The
                  ADXL345 powers up in standby mode with minimum power.
                  consumption.
@param sleep      Sleep mode suppresses DATA_READY, stops transmission of data
                  to FIFO, and switches the sampling rate to one specified by the
                  argument. In sleep mode, only the activity function can be used.
                  When the DATA_READY interrupt is suppressed, the output
                  data registers (Register 0x32 to Register 0x37) are still updated
                  at given sampling rate.
                  When clearing the sleep mode, it is recommended that the part be
                  placed into standby mode and then set back to measurement
                  mode with a subsequent write. This is done to ensure that the
                  device is properly biased if sleep mode is manually disabled;
                  otherwise, the first few samples of data after the sleep mode is
                  cleared may have additional noise, especially if the device was
                  asleep when the bit was cleared.
 */
int adxl_config_power(adxl345_dev *dev,
    bool low_power, adxl_rate rate,
    bool link, bool auto_sleep, adxl_mode mode,
    adxl_sleep sleep);

/**
Configure offsets.
@param dev      The adxl345 device struct.
@param x, y, z  The OFSX, OFSY, and OFSZ registers are each eight bits and
                offer user-set offset adjustments in twos complement format
                with a scale factor of 15.6 mg/LSB (that is, 0x7F = 2 g). The
                value stored in the offset registers is automatically added to the
                acceleration data, and the resulting value is stored in the output
                data registers.
 */
int adxl_set_offset(adxl345_dev *dev, s8_t x, s8_t y, s8_t z);

/**
Configure single/double tap detection.
@param dev      The adxl345 device struct.
@param tap_ena  Combination of axes enabled for tap detection.
@param thresh   The THRESH_TAP register is eight bits and holds the threshold
                value for tap interrupts. The data format is unsigned, therefore,
                the magnitude of the tap event is compared with the value
                in THRESH_TAP for normal tap detection. The scale factor is
                62.5 mg/LSB (that is, 0xFF = 16 g). A value of 0 may result in
                undesirable behavior if single tap/double tap interrupts are
                enabled.
@param dur      The DUR register is eight bits and contains an unsigned time
                value representing the maximum time that an event must be
                above the THRESH_TAP threshold to qualify as a tap event. The
                scale factor is 625 µs/LSB. A value of 0 disables the single tap/
                double tap functions,
@param latent   The latent register is eight bits and contains an unsigned time
                value representing the wait time from the detection of a tap
                event to the start of the time window (defined by the window
                register) during which a possible second tap event can be detected.
                The scale factor is 1.25 ms/LSB. A value of 0 disables the double tap
                function.
@param window   The window register is eight bits and contains an unsigned time
                value representing the amount of time after the expiration of the
                latency time (determined by the latent register) during which a
                second valid tap can begin. The scale factor is 1.25 ms/LSB. A
                value of 0 disables the double tap function.
@param suppress Setting the suppress bit suppresses double tap detection if
                acceleration greater than the value in THRESH_TAP is present
                between taps.
 */
int adxl_config_tap(adxl345_dev *dev,
    adxl_axes tap_ena, u8_t thresh, u8_t dur, u8_t latent, u8_t window, bool suppress);

/**
Configure activity/inactivity detection.
@param dev      The adxl345 device struct.
@param ac_dc    In dc-coupled operation, the current acceleration magnitude is
                compared directly with THRESH_ACT and THRESH_INACT to determine
                whether activity or inactivity is detected. In ac-coupled operation
                for activity detection, the acceleration value at the start of
                activity detection is taken as a reference value. New samples of
                acceleration are then compared to this reference value, and if the
                magnitude of the difference exceeds the THRESH_ACT value, the device
                triggers an activity interrupt.
                Similarly, in ac-coupled operation for inactivity detection, a
                reference value is used for comparison and is updated whenever
                the device exceeds the inactivity threshold. After the reference
                value is selected, the device compares the magnitude of the
                difference between the reference value and the current acceleration
                with THRESH_INACT. If the difference is less than the value in
                THRESH_INACT for the time in TIME_INACT, the device is
                considered inactive and the inactivity interrupt is triggered.
@param act_ena  Combination of axes enabled for activity triggering.
@param inact_ena  Combination of axes enabled for activity triggering.
@param thr_act  The THRESH_ACT register is eight bits and holds the threshold value for
                detecting activity. The data format is unsigned, so the magnitude of the
                activity event is compared with the value in the THRESH_ACT register.
                The scale factor is 62.5 mg/LSB. A value of 0 may result in undesirable
                behavior if the activity interrupt is enabled.
@param thr_inact  The THRESH_INACT register is eight bits and holds the threshold value
                for detecting inactivity. The data format is unsigned, so the magnitude of
                the inactivity event is compared with the value in the THRESH_INACT register.
                The scale factor is 62.5 mg/LSB. A value of 0 may result in undesirable
                behavior if the inactivity interrupt is enabled.
@param time_inact  The TIME_INACT register is eight bits and contains an unsigned time value
                representing the amount of time that acceleration must be less than the value
                in the THRESH_INACT register for inactivity to be declared. The scale factor
                is 1 sec/LSB. Unlike the other interrupt functions, which use unfiltered data,
                the inactivity function uses filtered output data.
                At least one output sample must be generated for the inactivity interrupt to
                be triggered. This results in the function appearing unresponsive if the
                TIME_INACT register is set to a value less than the time constant of the output
                data rate. A value of 0 results in an interrupt when the output data is less
                than the value in the THRESH_INACT register.
 */
int adxl_config_activity(adxl345_dev *dev,
    adxl_acdc ac_dc, adxl_axes act_ena, adxl_axes inact_ena, u8_t thr_act, u8_t thr_inact, u8_t time_inact);

/**
Configure freefall detection.
@param dev      The adxl345 device struct.
@param thresh   The THRESH_FF register is eight bits and holds the threshold
                value, in unsigned format, for free-fall detection. The acceleration on
                all axes is compared with the value in THRESH_FF to determine if
                a free-fall event occurred. The scale factor is 62.5 mg/LSB. Note
                that a value of 0 mg may result in undesirable behavior if the freefall
                interrupt is enabled. Values between 300 mg and 600 mg
                (0x05 to 0x09) are recommended.
@param time     The TIME_FF register is eight bits and stores an unsigned time
                value representing the minimum time that the value of all axes
                must be less than THRESH_FF to generate a free-fall interrupt.
                The scale factor is 5 ms/LSB. A value of 0 may result in undesirable
                behavior if the free-fall interrupt is enabled. Values between 100 ms
                and 350 ms (0x14 to 0x46) are recommended.
 */
int adxl_config_freefall(adxl345_dev *dev, u8_t thresh, u8_t time);

/**
Configure interrupts.
@param dev      The adxl345 device struct.
@param int_ena  Combinations of ADXL345_INT_* defines. The DATA_READY,
                watermark, and overrun bits enable only the interrupt output;
                the functions are always enabled. It is recommended that interrupts
                be configured before enabling their outputs.
@param int_map  Combinations of ADXL345_INT_* defines. Any bits set to 0 in this
                register send their respective interrupts to the INT1 pin, whereas
                bits set to 1 send their respective interrupts to the INT2 pin. All
                selected interrupts for a given pin are OR’ed.
 */
int adxl_config_interrupts(adxl345_dev *dev, u8_t int_ena, u8_t int_map);

/**
Configure format.
@param dev      The adxl345 device struct.
@param int_inv  A value of 0 in the INT_INVERT bit sets the interrupts to active
                high, and a value of 1 sets the interrupts to active low
@param full_res When this bit is set to a value of 1, the device is in full resolution
                mode, where the output resolution increases with the g range
                set by the range bits to maintain a 4 mg/LSB scale factor. When
                the FULL_RES bit is set to 0, the device is in 10-bit mode, and
                the range bits determine the maximum g range and scale factor.
@param justify  A setting of 1 in the justify bit selects left-justified (MSB) mode,
                and a setting of 0 selects right-justified mode with sign extension.
@param range    Sets the g range.
 */
int adxl_config_format(adxl345_dev *dev,
    bool int_inv, bool full_res, bool justify, adxl_range range);

/**
Configure FIFO.
@param dev      The adxl345 device struct.
@param mode     Bypass:  FIFO is bypassed.
                FIFO:    FIFO collects up to 32 values and then
                         stops collecting data, collecting new data
                         only when FIFO is not full.
                Stream:  FIFO holds the last 32 data values. When
                         FIFO is full, the oldest data is overwritten
                         with newer data.
                Trigger: When triggered by the trigger bit, FIFO
                         holds the last data samples before the
                         trigger event and then continues to collect
                         data until full. New data is collected only
                         when FIFO is not full.
@param trigger  A value of 0 in the trigger bit links the trigger event of trigger mode
                to INT1, and a value of 1 links the trigger event to INT2.
@param samples  The function of these bits depends on the FIFO mode selected.
                Entering a value of 0 in the samples bits immediately
                sets the watermark status bit in the INT_SOURCE register,
                regardless of which FIFO mode is selected. Undesirable operation
                may occur if a value of 0 is used for the samples bits when trigger
                mode is used.
                Bypass:   None.
                FIFO:    Specifies how many FIFO entries are needed to
                         trigger a watermark interrupt.
                Stream:  Specifies how many FIFO entries are needed to
                         trigger a watermark interrupt.
                Trigger: Specifies how many FIFO samples are retained in
                         the FIFO buffer before a trigger event.
 */
int adxl_config_fifo(adxl345_dev *dev,
    adxl_fifo_mode mode, adxl_pin_int trigger, u8_t samples);

/**
Read axes data.
@param dev      The adxl345 device struct.
@param data     Pointer to a data struct which will be populated with output data.
 */
int adxl_read_data(adxl345_dev *dev, adxl_reading *data);

/**
Read interrupt source.
Bits set to 1 in this register indicate that their respective functions
have triggered an event, whereas a value of 0 indicates that the
corresponding event has not occurred. The DATA_READY,
watermark, and overrun bits are always set if the corresponding
events occur, regardless of the INT_ENABLE register settings,
and are cleared by reading data from the DATAX, DATAY, and
DATAZ registers. The DATA_READY and watermark bits may
require multiple reads, as indicated in the FIFO mode descriptions
in the FIFO section. Other bits, and the corresponding interrupts,
are cleared by reading the INT_SOURCE register.
@param dev      The adxl345 device struct.
@param int_src  Pointer to a byte which will be populated with combination of
                ADXL345_INT_* defines.
 */
int adxl_read_interrupts(adxl345_dev *dev, u8_t *int_src);

/**
Read fifo status.
A 1 in the FIFO_TRIG bit corresponds to a trigger event occurring,
and a 0 means that a FIFO trigger event has not occurred.
Entries report how many data values are stored in FIFO.
Access to collect the data from FIFO is provided through the
DATAX, DATAY, and DATAZ registers. FIFO reads must be
done in burst or multiple-byte mode because each FIFO level is
cleared after any read (single- or multiple-byte) of FIFO. FIFO
stores a maximum of 32 entries, which equates to a maximum
of 33 entries available at any given time because an additional
entry is available at the output filter of the device.
@param dev      The adxl345 device struct.
@param int_src  Pointer to a fifo_status struct to be populated.
*/
int adxl_read_fifo_status(adxl345_dev *dev, adxl_fifo_status *fifo_status);

/**
Read activity/tap/sleep status
act_*  and tap_* bits indicate the first axis involved in a tap or activity
event. A setting of 1 corresponds to involvement in the event,
and a setting of 0 corresponds to no involvement. When new
data is available, these bits are not cleared but are overwritten by
the new data. The ACT_TAP_STATUS register should be read
before clearing the interrupt. Disabling an axis from participation
clears the corresponding source bit when the next activity or
single tap/double tap event occurs.
A setting of 1 in the asleep bit indicates that the part is asleep,
and a setting of 0 indicates that the part is not asleep. This bit
toggles only if the device is configured for auto sleep.
@param dev             The adxl345 device struct.
@param act_tap_status  Pointer to a act_tap_status struct to be populated.
*/
int adxl_read_act_tap_status(adxl345_dev *dev, adxl_act_tap_status *act_tap_status);

/**
Reads interrupt source, fifo status, and activity/tap status.
@param dev             The adxl345 device struct.
@param act_tap_status  Pointer to a status struct to be populated.
*/
int adxl_read_status(adxl345_dev *dev, adxl_status *status);

#endif /* ADXL345_DRIVER_H_ */
