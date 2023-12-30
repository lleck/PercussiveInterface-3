//################################################################################################ 
// A library for the TMAG5170 3d hall effect sensor heavily adopted from the
// driver for the MIKROE 3D HALL 10 CLICK add on board (https://www.mikroe.com/3d-hall-10-click),
// which employs the same sensor an is published under the MIT License.
// work in progress, do not expect flawless performance or arduino IDE integration.
//################################################################################################

#include "myTMAG5170.h"
#include <Arduino.h>

#define CLOCK_PIN 47
#define MOSI_PIN 48
#define MISO_PIN 14
#define DUMMY  0x00

TMAG5170::TMAG5170(){

}

// CRC4 function that calculates CRC4 from 4 input bytes (28-MSBs only) with 
// polynomial X4 + X + 1 and CRC initialization at 0x0F.
// input parameter: crc_source : 4 input bytes (28-MSBs only).
// returns calculated CRC4 output.
static uint8_t TMAG5170_calculate_crc ( uint8_t crc_source[ 4 ] );


void TMAG5170::begin(uint8_t chipSelectPin){
  setChipSelectPin(chipSelectPin);
  spiMaximumSpeed = 1000000;
  spiDataMode = SPI_MODE0;
  SPI.begin(CLOCK_PIN, MISO_PIN, MOSI_PIN, chipSelectPin);
  SPI.beginTransaction(SPISettings(spiMaximumSpeed, MSBFIRST, spiDataMode));
}

void TMAG5170::end() {
  SPI.end();
}

void TMAG5170::disable_crc() {
    uint8_t data_buf[4] = {0x0F,0x00,0x04,0x07};
    digitalWrite(spiChipSelectPin, LOW);
    // buffer array and size
    SPI.transfer(data_buf, 4);
    digitalWrite(spiChipSelectPin, HIGH);
}


void TMAG5170::setChipSelectPin(uint8_t chipSelectPin) {
  spiChipSelectPin = chipSelectPin;
  pinMode(spiChipSelectPin, OUTPUT);
  digitalWrite(spiChipSelectPin, HIGH);
}

// default configuration of the TMAG5170
void TMAG5170::default_cfg ( bool *error_detected )
{
    bool error = false;
   
    write_frame(TMAG5170_REG_DEVICE_CONFIG, TMAG5170_CONV_AVG_1X |               // additional sampling to reduce noise
                                     TMAG5170_MAG_TEMPCO_0p12 |                  // temp. coefficient of target magnet in %/C°
                                     TMAG5170_OPERATING_MODE_MEASURE |           // active trigger mode enabled 
                                     TMAG5170_T_CH_EN_DISABLE |                  // temp. channel disabled
                                     TMAG5170_T_RATE_PER_CONV_AVG |              // temp conversion rate same as other sensors
                                     TMAG5170_T_HLT_EN_DISABLE, &error);         // temperature limit check for high temp env

    write_frame(TMAG5170_REG_SENSOR_CONFIG, TMAG5170_ANGLE_EN_NO_ANGLE |         // no angle calculation (default)
                                     TMAG5170_SLEEPTIME_100MS |                  // sleeptime if standby operating mode 010b
                                     TMAG5170_MAG_CH_EN_ENABLE_Z |               // enables data acquisition of z axis channel
                                     TMAG5170_Z_RANGE_100mT , &error);            // set range for axis channels

    write_frame(TMAG5170_REG_SYSTEM_CONFIG, TMAG5170_DIAG_SEL_ALL_DP_DIAG_ALL |  // diagnostic mode on all data paths (default)
                                     TMAG5170_TRIGGER_MODE_CS_PULSE |            // conversion trigger condition 
                                     TMAG5170_DATA_TYPE_32BIT_REG |              // data type accessed from results registers (32bit default)
                                     TMAG5170_DIAG_EN_DISABLE |                  // user controlled AFE diagnostics
                                     TMAG5170_Z_HLT_EN_DISABLE |                 // magnetic field limit check for axis channels
                                     TMAG5170_Y_HLT_EN_DISABLE |
                                     TMAG5170_X_HLT_EN_DISABLE, &error); 

    write_frame(TMAG5170_REG_ALERT_CONFIG, TMAG5170_ALERT_LATCH_DISABLE, &error);// alert source latching state (irrelevant if alert pin nc)

    write_frame(TMAG5170_REG_MAG_GAIN_CONFIG, TMAG5170_GAIN_SELECTION_NO_AXIS, &error);
    if (error) {
     *error_detected = error;
      return;
    }
}



// Send out the Data to given Register with crc check 
void TMAG5170::write_frame (uint8_t reg_addr, uint16_t data_in, bool *error_detected ) 
{
    *error_detected = false;
    if ( reg_addr > TMAG5170_REG_MAGNITUDE_RESULT )
    {
        *error_detected = true;
    }
   //fill the buffer with data_in, append crc and transfer
    uint8_t data_buf[ 4 ] = { 0 };
    data_buf[ 0 ] = reg_addr & TMAG5170_SPI_WRITE_MASK;
    data_buf[ 1 ] = ( uint8_t ) ( ( data_in >> 8 ) & 0xFF );
    data_buf[ 2 ] = ( uint8_t ) ( data_in & 0xFF );
    data_buf[ 3 ] = TMAG5170_calculate_crc ( data_buf );
    digitalWrite(spiChipSelectPin, LOW);
    delay(1);
    // buffer array and size
    SPI.transfer(data_buf, 4);

    digitalWrite(spiChipSelectPin, HIGH);
}

void TMAG5170::simple_read(uint8_t reg_addr) {
    uint8_t val1 = 0;
    uint8_t val2 = 0;
    uint8_t crc = 0;
    uint8_t data_buf[ 4 ] = { 0 };
    data_buf[ 0 ] = reg_addr | TMAG5170_SPI_READ_MASK;
    data_buf[ 1 ] = DUMMY;
    data_buf[ 2 ] = DUMMY;
    data_buf[ 3 ] = DUMMY;

    digitalWrite(spiChipSelectPin, LOW);
    
        SPI.transfer(data_buf[0]);
        val1 = SPI.transfer(DUMMY);
        val2 = SPI.transfer(DUMMY);
        crc  = SPI.transfer(DUMMY);
   
    digitalWrite(spiChipSelectPin, HIGH);
    uint16_t data = ( ( uint16_t )val1 << 8 ) | val2;
         Serial.print("val1 ");
         Serial.println(val1, BIN);
         Serial.print("val2 ");
         Serial.println(val2, BIN);
         Serial.print("crc  ");
         Serial.println(crc, BIN); 
         Serial.println("DATA");
         Serial.println(data, BIN); 
         Serial.println("   ");


}

// Read out the Data of given Register with crc check and store in variable pointed by data_out
// declare local variables first and the set with memory adress e.g.: sensor.read_frame(reg_addr, &data_out, &status);
void TMAG5170::read_frame (uint8_t reg_addr, uint16_t *data_out, uint16_t *status, bool *error_detected ) 
{
    *error_detected = false;
    if ( reg_addr > TMAG5170_REG_MAGNITUDE_RESULT )
    {
        Serial.println("reg_addr error");
        *error_detected = true;
        return;
    }
    unsigned int ret = 0;
    uint8_t data_buf[ 4 ] = { 0 };
    data_buf[ 0 ] = reg_addr | TMAG5170_SPI_READ_MASK;
    data_buf[ 1 ] = DUMMY;
    data_buf[ 2 ] = DUMMY;
    data_buf[ 3 ] = TMAG5170_calculate_crc ( data_buf );
    digitalWrite(spiChipSelectPin, LOW);
    
    //SPI.transfer(data_buf, 4);
    for ( uint8_t cnt = 0; cnt < 4; cnt++ )
    {
         data_buf[cnt] = SPI.transfer(data_buf[cnt]);
    //     //spi_master_set_default_write_data( &ctx->spi, data_buf[ cnt ] );
    //     //error_flag |= spi_master_read( &ctx->spi, &data_buf[ cnt ], 1 );
    }
    digitalWrite(spiChipSelectPin, HIGH);
    // uint8_t crc = data_buf[ 3 ] & 0x0F;
    // if ( crc == TMAG5170_calculate_crc ( data_buf ) )
    // {   
    //     Serial.println("CRC validated");
        *status = ( ( uint16_t ) data_buf[ 0 ] << 4 ) | ( ( data_buf[ 3 ] >> 4 ) & 0x0F );
        *data_out = ( ( uint16_t ) data_buf[ 1 ] << 8 ) | data_buf[ 2 ];
        // ret = ( ( uint16_t ) data_buf[ 1 ] << 8 ) | data_buf[ 2 ];
        // Serial.println(ret);
    // }
    // else {
    //     *error_detected = true;
    // }   
}


float TMAG5170::getXresult( bool *error_detected ){
    uint16_t reg_status, reg_data, conv_status, sensor_config;
    float data;
    bool error = false;
    *error_detected = error;
    read_frame( TMAG5170_REG_CONV_STATUS, &conv_status, &reg_status, &error );
    if (!error && (conv_status & TMAG5170_CONV_STATUS_RDY))
    {
        read_frame( TMAG5170_REG_SENSOR_CONFIG, &sensor_config, &reg_status, &error );
        if (!error && ( conv_status & TMAG5170_CONV_STATUS_X ))
        {
            read_frame(TMAG5170_REG_X_CH_RESULT, &reg_data, &reg_status, &error );
            if (!error) 
            {    
                // uint16_t data_out is casted to a signed int and then casted to a float after division by the resolution
                // why does this work with an uint16_t containing only binary representation in 2's complement ?
                data = ( ( int16_t ) reg_data ) / TMAG5170_XYZ_RESOLUTION;
                switch ( sensor_config & TMAG5170_X_RANGE_BIT_MASK )
                {
                    case TMAG5170_X_RANGE_25mT:
                    {
                        data *= 25.0;
                        return data;
                    }
                    case TMAG5170_X_RANGE_50mT:
                    {
                        data *= 50.0;
                        return data;
                    }
                    case TMAG5170_X_RANGE_100mT:
                    {
                        data *= 100.0;
                        return data;
                    }
                }
            }
                    
        }
       
    }
    *error_detected = error;
    return data;
}

float TMAG5170::getYresult( bool *error_detected ){
    uint16_t reg_status, reg_data, conv_status, sensor_config;
    float data;
    bool error = false;
    *error_detected = error;
    read_frame( TMAG5170_REG_CONV_STATUS, &conv_status, &reg_status, &error );
    if (!error && (conv_status & TMAG5170_CONV_STATUS_RDY))
    {
        read_frame( TMAG5170_REG_SENSOR_CONFIG, &sensor_config, &reg_status, &error );
        if (!error && ( conv_status & TMAG5170_CONV_STATUS_Y ))
        {
            read_frame(TMAG5170_REG_Y_CH_RESULT, &reg_data, &reg_status, &error );
            if (!error) 
            {
                data = ( ( int16_t ) reg_data ) / TMAG5170_XYZ_RESOLUTION;
                switch ( sensor_config & TMAG5170_Y_RANGE_BIT_MASK )
                {
                    case TMAG5170_Y_RANGE_25mT:
                    {
                        data *= 25.0;
                        return data;
                    }
                    case TMAG5170_Y_RANGE_50mT:
                    {
                        data *= 50.0;
                        return data;
                    }
                    case TMAG5170_Y_RANGE_100mT:
                    {
                        data *= 100.0;
                        return data;
                    }
                }
            }
                    
        }
       
    }
    *error_detected = error;
    return data;
}

float TMAG5170::getZresult( bool *error_detected ){
    uint16_t reg_status, reg_data, conv_status, sensor_config;
    float data;
    bool error = false;
    *error_detected = error;
    read_frame( TMAG5170_REG_CONV_STATUS, &conv_status, &reg_status, &error );
    if (!error && (conv_status & TMAG5170_CONV_STATUS_RDY))
    {   
        read_frame( TMAG5170_REG_SENSOR_CONFIG, &sensor_config, &reg_status, &error );
        if (!error && ( conv_status & TMAG5170_CONV_STATUS_Z ))
        {   
            read_frame(TMAG5170_REG_Z_CH_RESULT, &reg_data, &reg_status, &error );
            if (!error) 
            {   
                data = ( ( int16_t ) reg_data ) / TMAG5170_XYZ_RESOLUTION;
                switch ( sensor_config & TMAG5170_Z_RANGE_BIT_MASK )
                {
                    case TMAG5170_Z_RANGE_25mT:
                    {
                        data *= 25.0;
                        return data;
                    }
                    case TMAG5170_Z_RANGE_50mT:
                    {
                        data *= 50.0;
                        return data;
                    }
                    case TMAG5170_Z_RANGE_100mT:
                    {
                        data *= 100.0;
                        return data;
                    }
                }
            }
                    
        }
       
    }
    *error_detected = error;
    return data;
}

float TMAG5170::getTEMPresult( bool *error_detected ){
   uint16_t reg_status, reg_data, conv_status, sensor_config;
    float data;
    bool error = false;
    *error_detected = error;
    read_frame( TMAG5170_REG_CONV_STATUS, &conv_status, &reg_status, &error );
    if (!error && (conv_status & TMAG5170_CONV_STATUS_RDY))
    {
        read_frame( TMAG5170_REG_SENSOR_CONFIG, &sensor_config, &reg_status, &error );
        if ( !error && (conv_status & TMAG5170_CONV_STATUS_T ))
        {
             read_frame (TMAG5170_REG_TEMP_RESULT, &reg_data, &reg_status, &error );
             if (!error)
             {
             data = TMAG5170_TEMP_SENS_T0 + ( ( int16_t ) reg_data - TMAG5170_TEMP_ADC_T0 ) / TMAG5170_TEMP_ADC_RESOLUTION;
             return data;
             }
        }
    }
    *error_detected = error;
    return data;
    
}

static uint8_t TMAG5170_calculate_crc ( uint8_t crc_source[ 4 ] )
{
    crc_source[ 3 ] &= 0xF0;
    uint8_t crc = 0x0F;
    for ( uint8_t byte_cnt = 0; byte_cnt < 4; byte_cnt++ )
    {
        for ( uint8_t bit_cnt = 8; bit_cnt > 0; bit_cnt-- )
        {
            uint8_t inv = ( ( crc_source[ byte_cnt ] >> ( bit_cnt - 1 ) ) & 1 ) ^ ( ( crc >> 3 ) & 1 );
            crc = ( ( crc ^ inv ) << 1 ) | inv;
        }
    }
    return crc & 0x0F;
}


// void TMAG517::read_all_data ( float *data_out, bool *error_detected )
// {
//     uint16_t reg_status, reg_data, conv_status, sensor_config;
//     *error_detected = false;
//     read_frame( TMAG5170_REG_CONV_STATUS, &conv_status, &reg_status );
//     if (conv_status & TMAG5170_CONV_STATUS_RDY)
//     {
//         read_frame( TMAG5170_REG_SENSOR_CONFIG, &sensor_config, &reg_status );
//         if ( conv_status & TMAG5170_CONV_STATUS_A )
//         {
//             read_frame ( TMAG5170_REG_ANGLE_RESULT, &reg_data, &reg_status );
//             data_out->angle = reg_data / TMAG5170_ANGLE_RESOLUTION;
//             read_frame ( TMAG5170_REG_MAGNITUDE_RESULT, &reg_data, &reg_status );
//             data_out->magnitude = reg_data;
//         }
//         if ( conv_status & TMAG5170_CONV_STATUS_T )
//         {
//            read_frame (TMAG5170_REG_TEMP_RESULT, &reg_data, &reg_status );
//             data_out->temperature = TMAG5170_TEMP_SENS_T0 + 
//                                     ( ( int16_t ) reg_data - TMAG5170_TEMP_ADC_T0 ) / TMAG5170_TEMP_ADC_RESOLUTION;
//         }
//         if ( conv_status & TMAG5170_CONV_STATUS_Z )
//         {
//             read_frame (TMAG5170_REG_Z_CH_RESULT, &reg_data, &reg_status );
//             data_out->z_axis = ( ( int16_t ) reg_data ) / TMAG5170_XYZ_RESOLUTION;
//             switch ( sensor_config & TMAG5170_Z_RANGE_BIT_MASK )
//             {
//                 case TMAG5170_Z_RANGE_25mT:
//                 {
//                     data_out->z_axis *= 25.0;
//                     break;
//                 }
//                 case TMAG5170_Z_RANGE_50mT:
//                 {
//                     data_out->z_axis *= 50.0;
//                     break;
//                 }
//                 case TMAG5170_Z_RANGE_100mT:
//                 {
//                     data_out->z_axis *= 100.0;
//                     break;
//                 }
//             }
//         }
//         if ( conv_status & TMAG5170_CONV_STATUS_Y )
//         {
//             error_flag |= TMAG5170_read_frame (TMAG5170_REG_Y_CH_RESULT, &reg_data, &reg_status );
//             data_out->y_axis = ( ( int16_t ) reg_data ) / TMAG5170_XYZ_RESOLUTION;
//             switch ( sensor_config & TMAG5170_Y_RANGE_BIT_MASK )
//             {
//                 case TMAG5170_Y_RANGE_25mT:
//                 {
//                     data_out->y_axis *= 25.0;
//                     break;
//                 }
//                 case TMAG5170_Y_RANGE_50mT:
//                 {
//                     data_out->y_axis *= 50.0;
//                     break;
//                 }
//                 case TMAG5170_Y_RANGE_100mT:
//                 {
//                     data_out->y_axis *= 100.0;
//                     break;
//                 }
//             }
//         }
//         if ( conv_status & TMAG5170_CONV_STATUS_X )
//         {
//             error_flag |= TMAG5170_read_frame (TMAG5170_REG_X_CH_RESULT, &reg_data, &reg_status );
//             data_out->x_axis = ( ( int16_t ) reg_data ) / TMAG5170_XYZ_RESOLUTION;
//             switch ( sensor_config & TMAG5170_X_RANGE_BIT_MASK )
//             {
//                 case TMAG5170_X_RANGE_25mT:
//                 {
//                     data_out->x_axis *= 25.0;
//                     break;
//                 }
//                 case TMAG5170_X_RANGE_50mT:
//                 {
//                     data_out->x_axis *= 50.0;
//                     break;
//                 }
//                 case TMAG5170_X_RANGE_100mT:
//                 {
//                     data_out->x_axis *= 100.0;
//                     break;
//                 }
//             }
//         }
//         return error_flag;
//     }
//     return TMAG5170_ERROR;
// }