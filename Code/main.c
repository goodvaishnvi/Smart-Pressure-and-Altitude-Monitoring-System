#include "ti_msp_dl_config.h"
#include <math.h>

/* =========================================================
 * LOW-POWER MONITORING SETTINGS
 * ========================================================= */
#define TEMP_CHANGE_LIMIT 10.0f     /* deg C */
#define PRESSURE_CHANGE_LIMIT 10.0f /* hPa */
#define ALTITUDE_CHANGE_LIMIT 20.0f /* meter */
#define OLED_DISPLAY_TIME 10        /* seconds */
/* SysTick is configured for 100 ms.
 * Ten SysTick interrupts = approximately 1 second.
 * MSPM0 SysTick period is limited, so do NOT use 32 MHz directly.
 */
#define SYSTICK_PERIOD_TICKS 3200000UL
#define SYSTICK_TICKS_PER_SAMPLE 10U
volatile unsigned char sample_flag = 0;
volatile unsigned char systick_count = 0;

/* =========================================================
 * I2C DEVICE ADDRESSES
 * ========================================================= */
#define OLED_ADDRESS 0x3C
#define BMP280_ADDRESS_1 0x76
#define BMP280_ADDRESS_2 0x77

/* =========================================================
 * BMP280 REGISTERS
 * ========================================================= */
#define BMP280_REG_ID 0xD0
#define BMP280_REG_RESET 0xE0
#define BMP280_REG_CTRL 0xF4
#define BMP280_REG_CONFIG 0xF5
#define BMP280_REG_PRESS 0xF7
#define BMP280_REG_TEMP 0xFA
#define BMP280_REG_CALIB 0x88

/* =========================================================
 * BMP280 VARIABLES
 * ========================================================= */
static unsigned char bmp280_address = BMP280_ADDRESS_1;
static unsigned short dig_T1;
static signed short dig_T2;
static signed short dig_T3;
static unsigned short dig_P1;
static signed short dig_P2;
static signed short dig_P3;
static signed short dig_P4;
static signed short dig_P5;
static signed short dig_P6;
static signed short dig_P7;
static signed short dig_P8;
static signed short dig_P9;
static signed long t_fine;

/* =========================================================
 * SMALL DELAY
 * ========================================================= */

void I2C_Delay(void)
{
    volatile unsigned int i;

    for (i = 0; i < 1000; i++)
    {
        __NOP();
    }
}

/* =========================================================
 * DELAY IN MILLISECONDS
 * ========================================================= */
void delay_ms(unsigned int ms)
{
    volatile unsigned int i;
    volatile unsigned int j;

    for (i = 0; i < ms; i++)
    {
        for (j = 0; j < 3000; j++)
        {
            __NOP();
        }
    }
}

/* =========================================================
 * GENERIC I2C WRITE
 * ========================================================= */
void I2C_Write2(unsigned char address,
                unsigned char byte1,
                unsigned char byte2)
{
    unsigned char buffer[2];
    buffer[0] = byte1;
    buffer[1] = byte2;
    while (!(DL_I2C_getControllerStatus(I2C1) &
             DL_I2C_CONTROLLER_STATUS_IDLE))
    {
    }
    DL_I2C_fillControllerTXFIFO(I2C1, buffer, 2);
    DL_I2C_startControllerTransfer(
        I2C1,
        address,
        DL_I2C_CONTROLLER_DIRECTION_TX,
        2);
    while (DL_I2C_getControllerStatus(I2C1) &
           DL_I2C_CONTROLLER_STATUS_BUSY_BUS)
    {
    }
}

/* =========================================================
 * OLED WRITE
 * ========================================================= */
void OLED_Write(unsigned char control,
                unsigned char data)
{
    I2C_Write2(
        OLED_ADDRESS,
        control,
        data);
}

/* =========================================================
 * OLED COMMAND
 * ========================================================= */
void OLED_Command(unsigned char command)
{
    OLED_Write(0x00, command);
}

/* =========================================================
 * OLED DISPLAY ON / OFF
 * ========================================================= */
void OLED_Wake(void)
{
    OLED_Command(0xAF); /* Display ON */
}
void OLED_Sleep(void)
{
    OLED_Command(0xAE); /* Display OFF */
}

/* =========================================================
 * SYSTICK INTERRUPT
 * ========================================================= */
void SysTick_Handler(void)
{
    systick_count++;

    if (systick_count >= SYSTICK_TICKS_PER_SAMPLE)
    {
        systick_count = 0;
        sample_flag = 1;
    }
}

/* =========================================================
 * SYSTICK INITIALIZATION
 * 100 ms interrupt, 10 interrupts = approximately 1 second
 * ========================================================= */
void SleepTimer_Init(void)
{
    DL_SYSTICK_init(SYSTICK_PERIOD_TICKS);
    DL_SYSTICK_enableInterrupt();
    DL_SYSTICK_enable();
}

/* =========================================================
 * OLED DATA
 * ========================================================= */
void OLED_Data(unsigned char data)
{
    OLED_Write(0x40, data);
}

/* =========================================================
 * OLED INITIALIZATION
 * ========================================================= */
void OLED_Init(void)
{
    delay_ms(100);
    OLED_Command(0xAE);
    OLED_Command(0x20);
    OLED_Command(0x02);
    OLED_Command(0xB0);
    OLED_Command(0xC8);
    OLED_Command(0x00);
    OLED_Command(0x10);
    OLED_Command(0x40);
    OLED_Command(0x81);
    OLED_Command(0x7F);
    OLED_Command(0xA1);
    OLED_Command(0xA6);
    OLED_Command(0xA8);
    OLED_Command(0x3F);
    OLED_Command(0xD3);
    OLED_Command(0x00);
    OLED_Command(0xD5);
    OLED_Command(0x80);
    OLED_Command(0xD9);
    OLED_Command(0xF1);
    OLED_Command(0xDA);
    OLED_Command(0x12);
    OLED_Command(0xDB);
    OLED_Command(0x40);
    OLED_Command(0x8D);
    OLED_Command(0x14);
    OLED_Command(0xAF);
    delay_ms(20);
}

/* =========================================================
 * OLED SET CURSOR
 * ========================================================= */
void OLED_SetCursor(unsigned char page,
                    unsigned char column)
{
    OLED_Command(0xB0 + page);

    OLED_Command(
        0x00 + (column & 0x0F));
    OLED_Command(
        0x10 + ((column >> 4) & 0x0F));
}

/* =========================================================
 * OLED CLEAR
 * ========================================================= */
void OLED_Clear(void)
{
    unsigned char page;
    unsigned char column;

    for (page = 0; page < 8; page++)
    {
        OLED_SetCursor(page, 0);

        for (column = 0; column < 128; column++)
        {
            OLED_Data(0x00);
        }
    }
}

/* =========================================================
 * OLED CHARACTER
 * 5x7 FONT
 * ========================================================= */
void OLED_Char(char c)
{
    unsigned char i;
    static const unsigned char space[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    static const unsigned char zero[5] = {0x3E, 0x51, 0x49, 0x45, 0x3E};
    static const unsigned char one[5] = {0x00, 0x42, 0x7F, 0x40, 0x00};
    static const unsigned char two[5] = {0x42, 0x61, 0x51, 0x49, 0x46};
    static const unsigned char three[5] = {0x21, 0x41, 0x45, 0x4B, 0x31};
    static const unsigned char four[5] = {0x18, 0x14, 0x12, 0x7F, 0x10};
    static const unsigned char five[5] = {0x27, 0x45, 0x45, 0x45, 0x39};
    static const unsigned char six[5] = {0x3C, 0x4A, 0x49, 0x49, 0x30};
    static const unsigned char seven[5] = {0x01, 0x71, 0x09, 0x05, 0x03};
    static const unsigned char eight[5] = {0x36, 0x49, 0x49, 0x49, 0x36};
    static const unsigned char nine[5] = {0x06, 0x49, 0x49, 0x29, 0x1E};
    static const unsigned char A[5] = {0x7E, 0x11, 0x11, 0x11, 0x7E};
    static const unsigned char B[5] = {0x7F, 0x49, 0x49, 0x49, 0x36};
    static const unsigned char C[5] = {0x3E, 0x41, 0x41, 0x41, 0x22};
    static const unsigned char E[5] = {0x7F, 0x49, 0x49, 0x49, 0x41};
    static const unsigned char K[5] = {0x7F, 0x08, 0x14, 0x22, 0x41};
    static const unsigned char M[5] = {0x7F, 0x02, 0x0C, 0x02, 0x7F};
    static const unsigned char N[5] = {0x7F, 0x04, 0x08, 0x10, 0x7F};
    static const unsigned char O[5] = {0x3E, 0x41, 0x41, 0x41, 0x3E};
    static const unsigned char P[5] = {0x7F, 0x09, 0x09, 0x09, 0x06};
    static const unsigned char R[5] = {0x7F, 0x09, 0x19, 0x29, 0x46};
    static const unsigned char S[5] = {0x46, 0x49, 0x49, 0x49, 0x31};
    static const unsigned char T[5] = {0x01, 0x01, 0x7F, 0x01, 0x01};
    static const unsigned char colon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
    static const unsigned char dot[5] = {0x00, 0x60, 0x60, 0x00, 0x00};
    static const unsigned char minus[5] = {0x08, 0x08, 0x08, 0x08, 0x08};
    const unsigned char *font;
    switch (c)
    {
    case '0':
        font = zero;
        break;
    case '1':
        font = one;
        break;
    case '2':
        font = two;
        break;
    case '3':
        font = three;
        break;
    case '4':
        font = four;
        break;
    case '5':
        font = five;
        break;
    case '6':
        font = six;
        break;
    case '7':
        font = seven;
        break;
    case '8':
        font = eight;
        break;
    case '9':
        font = nine;
        break;
    case 'A':
        font = A;
        break;
    case 'B':
        font = B;
        break;
    case 'C':
        font = C;
        break;
    case 'E':
        font = E;
        break;
    case 'K':
        font = K;
        break;
    case 'M':
        font = M;
        break;
    case 'N':
        font = N;
        break;
    case 'O':
        font = O;
        break;
    case 'P':
        font = P;
        break;
    case 'R':
        font = R;
        break;
    case 'S':
        font = S;
        break;
    case 'T':
        font = T;
        break;
    case ':':
        font = colon;
        break;
    case '.':
        font = dot;
        break;
    case '-':
        font = minus;
        break;
    default:
        font = space;
        break;
    }
    for (i = 0; i < 5; i++)
    {
        OLED_Data(font[i]);
    }
    OLED_Data(0x00);
}

/* =========================================================
 * OLED STRING
 * ========================================================= */
void OLED_String(const char *str)
{
    while (*str)
    {
        OLED_Char(*str);
        str++;
    }
}

/* =========================================================
 * OLED NUMBER
 * ========================================================= */
void OLED_Number(unsigned long number)
{
    char buffer[12];
    int i = 0;
    if (number == 0)
    {
        OLED_Char('0');
        return;
    }
    while (number > 0)
    {
        buffer[i++] =
            '0' + (number % 10);

        number /= 10;
    }
    while (i > 0)
    {
        i--;
        OLED_Char(buffer[i]);
    }
}

/* =========================================================
 * OLED SIGNED NUMBER
 * ========================================================= */
void OLED_SignedNumber(long number)
{
    if (number < 0)
    {
        OLED_Char('-');
        number = -number;
    }

    OLED_Number((unsigned long)number);
}

/* =========================================================
 * BMP280 WRITE REGISTER
 * ========================================================= */
void BMP280_WriteRegister(unsigned char reg,
                          unsigned char value)
{
    unsigned char buffer[2];
    buffer[0] = reg;
    buffer[1] = value;
    while (!(DL_I2C_getControllerStatus(I2C1) &
             DL_I2C_CONTROLLER_STATUS_IDLE))
    {
    }
    DL_I2C_fillControllerTXFIFO(
        I2C1,
        buffer, 2);
    DL_I2C_startControllerTransfer(
        I2C1,
        bmp280_address,
        DL_I2C_CONTROLLER_DIRECTION_TX, 2);
    while (DL_I2C_getControllerStatus(I2C1) &
           DL_I2C_CONTROLLER_STATUS_BUSY_BUS)
    {
    }
}

/* =========================================================
 * BMP280 READ BYTES
 * ========================================================= */
void BMP280_ReadBytes(unsigned char reg,
                      unsigned char *data,
                      unsigned int length)
{
    unsigned int i;
    while (!(DL_I2C_getControllerStatus(I2C1) &
             DL_I2C_CONTROLLER_STATUS_IDLE))
    {
    }
    /*
     * Send register address
     */
    DL_I2C_fillControllerTXFIFO(
        I2C1,
        &reg, 1);
    DL_I2C_startControllerTransfer(
        I2C1,
        bmp280_address,
        DL_I2C_CONTROLLER_DIRECTION_TX,
        1);
    while (DL_I2C_getControllerStatus(I2C1) &
           DL_I2C_CONTROLLER_STATUS_BUSY_BUS)
    {
    }

    /*
     * Read requested bytes
     */

    while (!(DL_I2C_getControllerStatus(I2C1) &
             DL_I2C_CONTROLLER_STATUS_IDLE))
    {
    }
    DL_I2C_startControllerTransfer(
        I2C1,
        bmp280_address,
        DL_I2C_CONTROLLER_DIRECTION_RX,
        length);
    for (i = 0; i < length; i++)
    {
        while (DL_I2C_isControllerRXFIFOEmpty(I2C1))
        {
        }

        data[i] =
            DL_I2C_receiveControllerData(I2C1);
    }

    while (DL_I2C_getControllerStatus(I2C1) &
           DL_I2C_CONTROLLER_STATUS_BUSY_BUS)
    {
    }
}

/* =========================================================
 * BMP280 READ SINGLE REGISTER
 * ========================================================= */
unsigned char BMP280_ReadRegister(unsigned char reg)
{
    unsigned char data;
    BMP280_ReadBytes(
        reg,
        &data,
        1);
    return data;
}

/* =========================================================
 * BMP280 DETECTION
 * ========================================================= */
unsigned char BMP280_Detect(void)
{
    unsigned char id;

    /*
     * Try 0x76
     */

    bmp280_address = BMP280_ADDRESS_1;

    id = BMP280_ReadRegister(
        BMP280_REG_ID);
    if ((id == 0x58) || (id == 0x56) || (id == 0x57))
    {
        return 1;
    }

    /*
     * Try 0x77
     */

    bmp280_address = BMP280_ADDRESS_2;
    id = BMP280_ReadRegister(
        BMP280_REG_ID);
    if ((id == 0x58) ||
        (id == 0x56) ||
        (id == 0x57))
    {
        return 1;
    }
    return 0;
}

/* =========================================================
 * BMP280 READ FACTORY CALIBRATION
 * ========================================================= */

void BMP280_ReadCalibration(void)
{
    unsigned char c[24];
    BMP280_ReadBytes(
        BMP280_REG_CALIB,
        c,
        24);

    dig_T1 = (unsigned short)(((unsigned short)c[1] << 8) | c[0]);
    dig_T2 = (signed short)(((unsigned short)c[3] << 8) | c[2]);
    dig_T3 = (signed short)(((unsigned short)c[5] << 8) | c[4]);
    dig_P1 = (unsigned short)(((unsigned short)c[7] << 8) | c[6]);
    dig_P2 = (signed short)(((unsigned short)c[9] << 8) | c[8]);
    dig_P3 = (signed short)(((unsigned short)c[11] << 8) | c[10])
        dig_P4 = (signed short)(((unsigned short)c[13] << 8) | c[12]);
    dig_P5 = (signed short)(((unsigned short)c[15] << 8) | c[14]);
    dig_P6 = (signed short)(((unsigned short)c[17] << 8) | c[16]);
    dig_P7 = (signed short)(((unsigned short)c[19] << 8) | c[18]);
    dig_P8 = (signed short)(((unsigned short)c[21] << 8) | c[20]);
    dig_P9 = (signed short)(((unsigned short)c[23] << 8) | c[22]);
}

/* =========================================================
 * BMP280 INITIALIZATION
 * ========================================================= */
void BMP280_Init(void)
{
    /*
     * CONFIG
     *
     * Standby = 500 ms
     * Filter = 2
     */
    BMP280_WriteRegister(
        BMP280_REG_CONFIG,
        0x08);
    /*
     * CTRL_MEAS
     *
     * Temperature x1
     * Pressure x1
     * Normal mode
     */
    BMP280_WriteRegister(
        BMP280_REG_CTRL,
        0x27);

    delay_ms(100);
}

/* =========================================================
 * BMP280 RAW TEMPERATURE
 * ========================================================= */
long BMP280_ReadRawTemperature(void)
{
    unsigned char d[3];
    long adc_T;
    BMP280_ReadBytes(
        BMP280_REG_TEMP,
        d, 3);

    adc_T = ((long)d[0] << 12) | ((long)d[1] << 4) | ((long)d[2] >> 4);
    return adc_T;
}

/* =========================================================
 * BMP280 RAW PRESSURE
 * ========================================================= */
long BMP280_ReadRawPressure(void)
{
    unsigned char d[3];
    long adc_P;

    BMP280_ReadBytes(
        BMP280_REG_PRESS,
        d, 3);

    adc_P = ((long)d[0] << 12) | ((long)d[1] << 4) | ((long)d[2] >> 4);
    return adc_P;
}

/* =========================================================
 * TEMPERATURE COMPENSATION
 *
 * IMPORTANT:
 * The final line intentionally ends with:
 *
 * ((signed long)dig_T3)) >> 14;
 *
 * NOT:
 *
 * ((signed long)dig_T3)) >> 14);
 * ========================================================= */

signed long BMP280_CompensateTemperature(long adc_T)
{
    signed long var1;
    signed long var2;
    signed long T;
    var1 = ((((adc_T >> 3) - ((signed long)dig_T1 << 1)) * ((signed long)dig_T2)) >> 11);
    var2 = (((((adc_T >> 4) - ((signed long)dig_T1)) * ((adc_T >> 4) - ((signed long)dig_T1))) >> 12) * ((signed long)dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

/* =========================================================
 * PRESSURE COMPENSATION
 *
 * Returns pressure in Pa
 * ========================================================= */

unsigned long BMP280_CompensatePressure(long adc_P)
{
    long long var1;
    long long var2;
    long long p;
    var1 = ((long long)t_fine) - 128000;
    var2 = var1 * var1 * (long long)dig_P6;
    var2 = var2 + ((var1 * (long long)dig_P5) << 17);
    var2 = var2 + (((long long)dig_P4) << 35);
    var1 = ((var1 * var1 * (long long)dig_P3) >> 8) + ((var1 * (long long)dig_P2) << 12);
    var1 = (((((long long)1) << 47) + var1) * (long long)dig_P1) >> 33;
    if (var1 == 0)
    {
        return 0;
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = ((long long)dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((long long)dig_P8 * p) >> 19;
    p = ((p + var1 + var2) >> 8) + ((long long)dig_P7 << 4);
    return (unsigned long)(p / 256);
}

/* =========================================================
 * READ TEMPERATURE
 * ========================================================= */
float BMP280_ReadTemperature(void)
{
    long raw_T;
    signed long temperature;
    raw_T =
        BMP280_ReadRawTemperature();

    temperature =
        BMP280_CompensateTemperature(
            raw_T);
    return ((float)temperature / 100.0f);
}

/* =========================================================
 * READ PRESSURE
 *
 * Returns hPa
 * ========================================================= */
float BMP280_ReadPressure(void)
{
    long raw_P;
    unsigned long pressure;
    raw_P = BMP280_ReadRawPressure();
    pressure = BMP280_CompensatePressure(raw_P);
    return ((float)pressure / 100.0f);
}

/* =========================================================
 * ALTITUDE
 * ========================================================= */
float BMP280_CalculateAltitude(float pressure_hPa)
{
    const float P0 = 1013.25f;
    if (pressure_hPa <= 0.0f)
    {
        return 0.0f;
    }
    return 44330.0f *
           (1.0f - powf(pressure_hPa / P0, 0.1903f));
}

/* =========================================================
 * DISPLAY PRESSURE
 * ========================================================= */
void OLED_DisplayPressure(float pressure)
{
    unsigned long p100;
    p100 = (unsigned long)(pressure * 100.0f);
    OLED_SetCursor(0, 0);
    OLED_String("P:");
    OLED_Number(p100 / 100);
    OLED_Char('.');
    OLED_Char(
        '0' + ((p100 / 10) % 10));
    OLED_Char(
        '0' + (p100 % 10));
}

/* =========================================================
 * DISPLAY TEMPERATURE
 * ========================================================= */
void OLED_DisplayTemperature(float temperature)
{
    long t100;
    t100 = (long)(temperature * 100.0f);
    OLED_SetCursor(2, 0);
    OLED_String("T:");
    if (t100 < 0)
    {
        OLED_Char('-');
        t100 = -t100;
    }
    OLED_Number((unsigned long)(t100 / 100));
    OLED_Char('.');
    OLED_Char(
        '0' + ((t100 / 10) % 10));
    OLED_Char(
        '0' + (t100 % 10));
    OLED_Char('C');
}

/* =========================================================
 * DISPLAY ALTITUDE
 * ========================================================= */
void OLED_DisplayAltitude(float altitude)
{
    long altitude_int;
    altitude_int = (long)altitude;
    OLED_SetCursor(4, 0);
    OLED_String("A:");
    OLED_SignedNumber(
        altitude_int);
    OLED_Char('M');
}

/* =========================================================
 * BMP280 STATUS OK
 * ========================================================= */

void OLED_DisplayBMP_OK(void)
{
    OLED_SetCursor(6, 0);
    OLED_String("BMP280 OK");
}

/* =========================================================
 * BMP280 ERROR
 * ========================================================= */
void OLED_DisplayBMP_ERROR(void)
{
    OLED_Clear();
    OLED_SetCursor(2, 0);
    OLED_String("BMP280");
    OLED_SetCursor(4, 0);
    OLED_String("NOT FOUND");
}

/* =========================================================
 * MAIN
 * ========================================================= */
int main(void)
{
    float pressure;
    float temperature;
    float altitude;
    float old_pressure;
    float old_temperature;
    float old_altitude;
    unsigned char first_reading = 1;
    unsigned char oled_active = 0;
    unsigned int oled_timer = 0;
    unsigned char display_refreshed = 0;

    /* =====================================================
     * INITIALIZE MSPM0 PERIPHERALS
     * ===================================================== */

    SYSCFG_DL_init();

    /* PA0 LED is active-low: start with LED OFF. */
    DL_GPIO_setPins(
        GPIO_USER_LED_1_PORT,
        GPIO_USER_LED_1_USER_LED_1_PIN);
    delay_ms(100);

    /* =====================================================
     * INITIALIZE OLED
     * ===================================================== */
    OLED_Init();
    OLED_Clear();

    /* =====================================================
     * STARTUP MESSAGE
     * ===================================================== */
    OLED_SetCursor(2, 0);
    OLED_String("PRESSURE");
    OLED_SetCursor(4, 0);
    OLED_String("MONITOR");
    delay_ms(1000);
    OLED_Clear();

    /* =====================================================
     * BMP280 DETECTION
     * ===================================================== */
    if (BMP280_Detect() == 0)
    {
        OLED_DisplayBMP_ERROR();
        while (1)
        {
            /* Sensor not found */
            __WFI();
        }
    }

    /* =====================================================
     * READ BMP280 FACTORY CALIBRATION
     * ===================================================== */
    BMP280_ReadCalibration();

    /* =====================================================
     * INITIALIZE BMP280
     * ===================================================== */
    BMP280_Init();

    /* =====================================================
     * FIRST SENSOR READING
     * ===================================================== */
    temperature = BMP280_ReadTemperature();
    pressure = BMP280_ReadPressure();
    altitude = BMP280_CalculateAltitude(pressure);
    old_temperature = temperature;
    old_pressure = pressure;
    old_altitude = altitude;

    /* =====================================================
     * DISPLAY FIRST READING
     * ===================================================== */
    OLED_Clear();
    OLED_DisplayPressure(pressure);
    OLED_DisplayTemperature(temperature);
    OLED_DisplayAltitude(altitude);
    OLED_DisplayBMP_OK();
    delay_ms(5000);

    /* =====================================================
     * OLED OFF
     * ===================================================== */
    OLED_Sleep();
    oled_active = 0;

    /* =====================================================
     * INITIALIZE LOW-POWER TIMER
     * ===================================================== */
    SleepTimer_Init();

    /* =====================================================
     * LOW-POWER MONITORING LOOP
     * ===================================================== */
    while (1)
    {
        /* =================================================
         * WAIT FOR THE NEXT TIMER INTERRUPT
         *
         * LED behavior:
         *   OLED OFF  -> LED ON  (MCU waiting)
         *   OLED ON   -> LED OFF (displaying data)
         *
         * PA0 LED is active-low on this board.
         * ================================================= */
        while (sample_flag == 0)
        {
            if (oled_active)
            {
                /* OLED is ON, so LED must remain OFF. */
                DL_GPIO_setPins(
                    GPIO_USER_LED_1_PORT,
                    GPIO_USER_LED_1_USER_LED_1_PIN);
            }
            else
            {
                /* OLED is OFF, so LED indicates waiting/sleep. */
                DL_GPIO_clearPins(
                    GPIO_USER_LED_1_PORT,
                    GPIO_USER_LED_1_USER_LED_1_PIN);
            }

            /* Wait for the 1-second timer interrupt. */
            __WFI();
        }

        /* =================================================
         * TIMER INTERRUPT OCCURRED - MCU WAKES UP
         * ================================================= */
        /* LED OFF while MCU is reading/processing. */
        DL_GPIO_setPins(
            GPIO_USER_LED_1_PORT,
            GPIO_USER_LED_1_USER_LED_1_PIN);
        sample_flag = 0;
        display_refreshed = 0;

        /* =================================================
         * READ BMP280
         * ================================================= */
        temperature = BMP280_ReadTemperature();
        pressure = BMP280_ReadPressure();
        altitude = BMP280_CalculateAltitude(pressure);

        /* =================================================
         * CHECK FOR SIGNIFICANT CHANGE
         * ================================================= */
        if (first_reading)
        {
            old_temperature = temperature;
            old_pressure = pressure;
            old_altitude = altitude;

            first_reading = 0;
        }
        if ((fabsf(temperature - old_temperature) >= TEMP_CHANGE_LIMIT) ||
            (fabsf(pressure - old_pressure) >= PRESSURE_CHANGE_LIMIT) ||
            (fabsf(altitude - old_altitude) >= ALTITUDE_CHANGE_LIMIT))
        {
            /* =============================================
             * SIGNIFICANT CHANGE DETECTED
             * ============================================= */
            /* Make sure LED is OFF before OLED turns ON. */
            DL_GPIO_setPins(
                GPIO_USER_LED_1_PORT,
                GPIO_USER_LED_1_USER_LED_1_PIN);
            OLED_Wake();
            OLED_Clear();
            OLED_DisplayPressure(pressure);
            OLED_DisplayTemperature(temperature);
            OLED_DisplayAltitude(altitude);
            OLED_DisplayBMP_OK();

            /*
             * Keep OLED ON for OLED_DISPLAY_TIME seconds.
             * The timer is checked once every second.
             */
            oled_active = 1;
            oled_timer = OLED_DISPLAY_TIME;
            display_refreshed = 1;

            /* Update reference values only after a displayed change. */
            old_temperature = temperature;
            old_pressure = pressure;
            old_altitude = altitude;
        }

        /* =================================================
         * OLED DISPLAY TIMER
         * ================================================= */
        if (oled_active && !display_refreshed)
        {
            if (oled_timer > 0)
            {
                oled_timer--;
            }
            if (oled_timer == 0)
            {
                OLED_Sleep();
                oled_active = 0;
            }
        }

        /* =================================================
         * LED STATE AFTER PROCESSING
         * ================================================= */
        if (oled_active)
        {
            /* OLED is still ON -> LED must stay OFF. */
            DL_GPIO_setPins(
                GPIO_USER_LED_1_PORT,
                GPIO_USER_LED_1_USER_LED_1_PIN);
        }
        else
        {
            /* OLED is OFF -> LED ON while waiting for next sample. */
            DL_GPIO_clearPins(
                GPIO_USER_LED_1_PORT,
                GPIO_USER_LED_1_USER_LED_1_PIN);
        }
    }
}
