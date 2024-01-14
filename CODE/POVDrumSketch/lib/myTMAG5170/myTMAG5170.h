//################################################################################################ 
// A library for the TMAG5170 3d hall effect sensor heavily adopted from the
// driver for the MIKROE 3D HALL 10 CLICK add on board (https://www.mikroe.com/3d-hall-10-click),
// which employs the same sensor an is published under the MIT License.
// work in progress, do not expect flawless performance or arduino IDE integration.
//################################################################################################

#ifndef MYTMAG5170_H
#define MYTMAG5170_H
#include <SPI.h>


//TMAG5170 Register Address List 
#define TMAG5170_REG_DEVICE_CONFIG             0x00
#define TMAG5170_REG_SENSOR_CONFIG             0x01
#define TMAG5170_REG_SYSTEM_CONFIG             0x02
#define TMAG5170_REG_ALERT_CONFIG              0x03
#define TMAG5170_REG_X_THRX_CONFIG             0x04
#define TMAG5170_REG_Y_THRX_CONFIG             0x05
#define TMAG5170_REG_Z_THRX_CONFIG             0x06
#define TMAG5170_REG_T_THRX_CONFIG             0x07
#define TMAG5170_REG_CONV_STATUS               0x08
#define TMAG5170_REG_X_CH_RESULT               0x09
#define TMAG5170_REG_Y_CH_RESULT               0x0A
#define TMAG5170_REG_Z_CH_RESULT               0x0B
#define TMAG5170_REG_TEMP_RESULT               0x0C
#define TMAG5170_REG_AFE_STATUS                0x0D
#define TMAG5170_REG_SYS_STATUS                0x0E
#define TMAG5170_REG_TEST_CONFIG               0x0F
#define TMAG5170_REG_OSC_MONITOR               0x10
#define TMAG5170_REG_MAG_GAIN_CONFIG           0x11
#define TMAG5170_REG_MAG_OFFSET_CONFIG         0x12
#define TMAG5170_REG_ANGLE_RESULT              0x13
#define TMAG5170_REG_MAGNITUDE_RESULT          0x14

//TMAG5170 Device Config Register List 
#define TMAG5170_CONV_AVG_1X                   0x0000
#define TMAG5170_CONV_AVG_2X                   0x1000
#define TMAG5170_CONV_AVG_4X                   0x2000
#define TMAG5170_CONV_AVG_8X                   0x3000
#define TMAG5170_CONV_AVG_16X                  0x4000
#define TMAG5170_CONV_AVG_32X                  0x5000
#define TMAG5170_CONV_AVG_BIT_MASK             0x7000
#define TMAG5170_MAG_TEMPCO_0                  0x0000
#define TMAG5170_MAG_TEMPCO_0p12               0x0100
#define TMAG5170_MAG_TEMPCO_0p03               0x0200
#define TMAG5170_MAG_TEMPCO_0p2                0x0300
#define TMAG5170_MAG_TEMPCO_BIT_MASK           0x0300
#define TMAG5170_OPERATING_MODE_CONFIG         0x0000
#define TMAG5170_OPERATING_MODE_STANDBY        0x0010
#define TMAG5170_OPERATING_MODE_MEASURE        0x0020
#define TMAG5170_OPERATING_MODE_TRIGGER        0x0030
#define TMAG5170_OPERATING_MODE_DUTY_CYCLED    0x0040
#define TMAG5170_OPERATING_MODE_SLEEP          0x0050
#define TMAG5170_OPERATING_MODE_DEEP_SLEEP     0x0060
#define TMAG5170_OPERATING_MODE_BIT_MASK       0x0070
#define TMAG5170_T_CH_EN_DISABLE               0x0000
#define TMAG5170_T_CH_EN_ENABLE                0x0008
#define TMAG5170_T_CH_EN_BIT_MASK              0x0008
#define TMAG5170_T_RATE_PER_CONV_AVG           0x0000
#define TMAG5170_T_RATE_ONCE_PER_CONV          0x0004
#define TMAG5170_T_RATE_BIT_MASK               0x0004
#define TMAG5170_T_HLT_EN_DISABLE              0x0000
#define TMAG5170_T_HLT_EN_ENABLE               0x0002
#define TMAG5170_T_HLT_EN_BIT_MASK             0x0002

// //TMAG5170 Sensor Config Register List 
#define TMAG5170_ANGLE_EN_NO_ANGLE             0x0000
#define TMAG5170_ANGLE_EN_XY_ANGLE             0x4000
#define TMAG5170_ANGLE_EN_YZ_ANGLE             0x8000
#define TMAG5170_ANGLE_EN_XZ_ANGLE             0xC000
#define TMAG5170_ANGLE_EN_BIT_MASK             0xC000
#define TMAG5170_SLEEPTIME_1MS                 0x0000
#define TMAG5170_SLEEPTIME_5MS                 0x0400
#define TMAG5170_SLEEPTIME_10MS                0x0800
#define TMAG5170_SLEEPTIME_15MS                0x0C00
#define TMAG5170_SLEEPTIME_20MS                0x1000
#define TMAG5170_SLEEPTIME_30MS                0x1400
#define TMAG5170_SLEEPTIME_50MS                0x1800
#define TMAG5170_SLEEPTIME_100MS               0x1C00
#define TMAG5170_SLEEPTIME_500MS               0x2000
#define TMAG5170_SLEEPTIME_1000MS              0x2400
#define TMAG5170_SLEEPTIME_BIT_MASK            0x3C00
#define TMAG5170_MAG_CH_EN_DISABLE             0x0000
#define TMAG5170_MAG_CH_EN_ENABLE_X            0x0040
#define TMAG5170_MAG_CH_EN_ENABLE_Y            0x0080
#define TMAG5170_MAG_CH_EN_ENABLE_XY           0x00C0
#define TMAG5170_MAG_CH_EN_ENABLE_Z            0x0100
#define TMAG5170_MAG_CH_EN_ENABLE_ZX           0x0140
#define TMAG5170_MAG_CH_EN_ENABLE_YZ           0x0180
#define TMAG5170_MAG_CH_EN_ENABLE_XYZ          0x01C0
#define TMAG5170_MAG_CH_EN_ENABLE_XYX          0x0200
#define TMAG5170_MAG_CH_EN_ENABLE_YXY          0x0240
#define TMAG5170_MAG_CH_EN_ENABLE_YZY          0x0280
#define TMAG5170_MAG_CH_EN_ENABLE_ZYZ          0x02C0
#define TMAG5170_MAG_CH_EN_ENABLE_ZXZ          0x0300
#define TMAG5170_MAG_CH_EN_ENABLE_XZX          0x0340
#define TMAG5170_MAG_CH_EN_ENABLE_XYZYX        0x0380
#define TMAG5170_MAG_CH_EN_ENABLE_XYZZYX       0x03C0
#define TMAG5170_MAG_CH_EN_BIT_MASK            0x03C0
#define TMAG5170_Z_RANGE_50mT                  0x0000
#define TMAG5170_Z_RANGE_25mT                  0x0010
#define TMAG5170_Z_RANGE_100mT                 0x0020
#define TMAG5170_Z_RANGE_BIT_MASK              0x0030
#define TMAG5170_Y_RANGE_50mT                  0x0000
#define TMAG5170_Y_RANGE_25mT                  0x0004
#define TMAG5170_Y_RANGE_100mT                 0x0008
#define TMAG5170_Y_RANGE_BIT_MASK              0x000C
#define TMAG5170_X_RANGE_50mT                  0x0000
#define TMAG5170_X_RANGE_25mT                  0x0001
#define TMAG5170_X_RANGE_100mT                 0x0002
#define TMAG5170_X_RANGE_BIT_MASK              0x0003

// System Config Register List
#define TMAG5170_DIAG_SEL_ALL_DP_DIAG_ALL      0x0000
#define TMAG5170_DIAG_SEL_EN_DP_ONLY_DIAG_ALL  0x1000
#define TMAG5170_DIAG_SEL_ALL_DP_DIAG_SEQ      0x2000
#define TMAG5170_DIAG_SEL_EN_DP_ONLY_DIAG_SEQ  0x3000
#define TMAG5170_DIAG_SEL_BIT_MASK             0x3000
#define TMAG5170_TRIGGER_MODE_SPI_CMD          0x0000
#define TMAG5170_TRIGGER_MODE_CS_PULSE         0x0200
#define TMAG5170_TRIGGER_MODE_ALERT_PULSE      0x0400
#define TMAG5170_TRIGGER_MODE_BIT_MASK         0x0600
#define TMAG5170_DATA_TYPE_32BIT_REG           0x0000
#define TMAG5170_DATA_TYPE_12BIT_XY_DATA       0x0040
#define TMAG5170_DATA_TYPE_12BIT_XZ_DATA       0x0080
#define TMAG5170_DATA_TYPE_12BIT_ZY_DATA       0x00C0
#define TMAG5170_DATA_TYPE_12BIT_XT_DATA       0x0100
#define TMAG5170_DATA_TYPE_12BIT_YT_DATA       0x0140
#define TMAG5170_DATA_TYPE_12BIT_ZT_DATA       0x0180
#define TMAG5170_DATA_TYPE_12BIT_AM_DATA       0x01C0
#define TMAG5170_DATA_TYPE_BIT_MASK            0x01C0
#define TMAG5170_DIAG_EN_DISABLE               0x0000
#define TMAG5170_DIAG_EN_ENABLE                0x0020
#define TMAG5170_DIAG_EN_BIT_MASK              0x0020
#define TMAG5170_Z_HLT_EN_DISABLE              0x0000
#define TMAG5170_Z_HLT_EN_ENABLE               0x0004
#define TMAG5170_Z_HLT_EN_BIT_MASK             0x0004
#define TMAG5170_Y_HLT_EN_DISABLE              0x0000
#define TMAG5170_Y_HLT_EN_ENABLE               0x0002
#define TMAG5170_Y_HLT_EN_BIT_MASK             0x0002
#define TMAG5170_X_HLT_EN_DISABLE              0x0000
#define TMAG5170_X_HLT_EN_ENABLE               0x0001
#define TMAG5170_X_HLT_EN_BIT_MASK             0x0001

// Alert Function Config Register List
#define TMAG5170_ALERT_LATCH_DISABLE           0x0000
#define TMAG5170_ALERT_LATCH_ENABLE            0x2000
#define TMAG5170_ALERT_LATCH_BIT_MASK          0x2000
#define TMAG5170_ALERT_MODE_INTERRUPT          0x0000
#define TMAG5170_ALERT_MODE_SWITCH             0x1000
#define TMAG5170_ALERT_MODE_BIT_MASK           0x1000
#define TMAG5170_STATUS_ALRT_AFE_SYS_NO_ASSERT 0x0000
#define TMAG5170_STATUS_ALRT_AFE_SYS_ASSERT    0x0800
#define TMAG5170_STATUS_ALRT_BIT_MASK          0x0800
#define TMAG5170_RSLT_ALRT_NO_CONV_COMPLETE    0x0000
#define TMAG5170_RSLT_ALRT_CONV_COMPLETE       0x0100
#define TMAG5170_RSLT_ALRT_BIT_MASK            0x0100
#define TMAG5170_THRX_COUNT_1_CONV             0x0000
#define TMAG5170_THRX_COUNT_2_CONV             0x0010
#define TMAG5170_THRX_COUNT_3_CONV             0x0020
#define TMAG5170_THRX_COUNT_4_CONV             0x0030
#define TMAG5170_THRX_COUNT_BIT_MASK           0x0030
#define TMAG5170_T_THRX_ALRT_NO_CROSSED        0x0000
#define TMAG5170_T_THRX_ALRT_CROSSED           0x0008
#define TMAG5170_T_THRX_ALRT_BIT_MASK          0x0008
#define TMAG5170_Z_THRX_ALRT_NO_CROSSED        0x0000
#define TMAG5170_Z_THRX_ALRT_CROSSED           0x0004
#define TMAG5170_Z_THRX_ALRT_BIT_MASK          0x0004
#define TMAG5170_Y_THRX_ALRT_NO_CROSSED        0x0000
#define TMAG5170_Y_THRX_ALRT_CROSSED           0x0002
#define TMAG5170_Y_THRX_ALRT_BIT_MASK          0x0002
#define TMAG5170_X_THRX_ALRT_NO_CROSSED        0x0000
#define TMAG5170_X_THRX_ALRT_CROSSED           0x0001
#define TMAG5170_X_THRX_ALRT_BIT_MASK          0x0001

// Mag Gain Config Register List
#define TMAG5170_GAIN_SELECTION_NO_AXIS        0x0000
#define TMAG5170_GAIN_SELECTION_X_AXIS         0x4000
#define TMAG5170_GAIN_SELECTION_Y_AXIS         0x8000
#define TMAG5170_GAIN_SELECTION_Z_AXIS         0xC000
#define TMAG5170_GAIN_SELECTION_BIT_MASK       0xC000
#define TMAG5170_GAIN_VALUE_BIT_MASK           0x07FF

// Conv Status Register List
#define TMAG5170_CONV_STATUS_RDY               0x2000
#define TMAG5170_CONV_STATUS_A                 0x1000
#define TMAG5170_CONV_STATUS_T                 0x0800
#define TMAG5170_CONV_STATUS_Z                 0x0400
#define TMAG5170_CONV_STATUS_Y                 0x0200
#define TMAG5170_CONV_STATUS_X                 0x0100
#define TMAG5170_CONV_STATUS_SET_COUNT         0x0070
#define TMAG5170_CONV_STATUS_ALRT_STATUS       0x0003

// Specified calculation values 
#define TMAG5170_ANGLE_RESOLUTION              16.0
#define TMAG5170_TEMP_SENS_T0                  25.0
#define TMAG5170_TEMP_ADC_T0                   17522
#define TMAG5170_TEMP_ADC_RESOLUTION           60.0
#define TMAG5170_XYZ_RESOLUTION                32768


#define TMAG5170_SPI_READ_MASK                 0x80
#define TMAG5170_SPI_WRITE_MASK                0x7F




class TMAG5170 {
public:
    TMAG5170();
    void begin(uint8_t chipSelectPin);
    void end();
    void disable_crc();
    void setChipSelectPin(uint8_t chipSelectPin);
    void default_cfg(bool *error_detected);
    void write_frame (uint8_t reg_addr, uint16_t data_in, bool *error_detected );
    void read_frame  (uint8_t reg_addr, uint16_t *data_out, uint16_t *status, bool *error_detected );
    void simple_read (uint8_t reg_addr);
    float getXresult(bool *error_detected);
    float getYresult(bool *error_detected);
    int getZresult(bool *error_detected);
    float getTEMPresult(bool *error_detected);
    // float getANGLEresult();
    // float getMAGresult();



private:
    uint32_t spiMaximumSpeed;
    uint8_t  spiDataMode;
    uint8_t  spiChipSelectPin;
};

#endif //MYTMAG5170_H