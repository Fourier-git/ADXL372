#include "ADXL372.h"

#define DEVID_PRODUCT 0xFA //372 in octal :) 

// Data registers. Each axis data has a 12 bit value. Data is left justified, MSBFIRST.
// Register *_H contains the eight most significant bits (MSBs), and Register *_L contains the four least significant bits (LSBs) of the 12-bit value
#define XDATA_H 0x08
#define XDATA_L 0x09
#define YDATA_H 0x0A
#define YDATA_L 0x0B
#define ZDATA_H 0x0C
#define ZDATA_L 0x0D

// ID registers
#define DEVID_AD 0x00
#define DEVID_MST 0x01
#define PARTID 0x02
#define REVID 0x03

// System registers
#define STATUS 0x04 // Status register
#define TIMING 0x3D // Timing control register
#define MEASURE 0x3E // Measurement control register
#define POWER_CTL 0x3F // Power control register

// System bitmasks
#define EXT_SYNC_MASK 0xFE // Timing
#define EXT_CLK_MASK 0xFD
#define WAKEUP_RATE_MASK 0xE3 
#define ODR_MASK 0x1F

#define BANDWIDTH_MASK 0xF8 // Measure
#define LOW_NOISE_MASK 0xF7
#define LINKLOOP_MASK 0xCF
#define AUTOSLEEP_MASK 0xBF 

#define MODE_MASK 0xFC // Power Control
#define HPF_DISABLE_MASK 0xFB
#define LPF_DISABLE_MASK 0xF7
#define FILTER_SETTLE_MASK 0xEF
#define INSTANT_ON_THRESH_MASK 0xDF

// Accelerometer Constants
#define SPI_SPEED 10000000 // ADXL372 supports up to 10MHz in SCLK frequency
#define SCALE_FACTOR 100 // mg per LSB
#define MG_TO_G 0.001 // g per mg

ADXL372class::ADXL372class(int csPinInput)
{
    m_csPin = csPinInput;
}

ADXL372class::~ADXL372class()
{
}

void ADXL372class::begin()
{
    SPI.begin();
    SPI.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE0)); // CPHA = CPOL = 0
    pinMode(m_csPin, OUTPUT); // Setting chip select pin
    digitalWrite(m_csPin, HIGH); // Pin ready

}

void ADXL372class::begin(Bandwidth bandwidth, Odr odr)
{
    SPI.begin();
    SPI.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE0)); // CPHA = CPOL = 0
    pinMode(m_csPin, OUTPUT); // Setting chip select pin
    digitalWrite(m_csPin, HIGH); // Pin ready

    setBandwidth(bandwidth);
    setOdr(odr);

}

void ADXL372class::end()
{
    //set some adresses here

    SPI.end();
}

void ADXL372class::printDevice(){
    byte devidAd = readRegister(DEVID_AD);
    byte devidMst = readRegister(DEVID_MST);
    byte partId = readRegister(PARTID);
    byte revId = readRegister(REVID);
    byte status = readRegister(STATUS);

    Serial.print("DEVID_AD: 0x");
    Serial.println(devidAd, HEX);

    Serial.print("DEVID_MST: 0x");
    Serial.println(devidMst, HEX);

    Serial.print("PARTID: 0x");
    Serial.println(partId, HEX);

    Serial.print("REVID: 0x");
    Serial.println(revId, HEX);
    
    Serial.print("STATUS: 0x");
    Serial.println(status, HEX);
}

void ADXL372class::readAcceleration(float& x, float& y, float& z, bool statusCheck) {
    
    if (statusCheck == true)
    {
        byte status;
        do {
            status = readRegister(0x04);
        } while((status & 0x01) == 0); // Waiting for status register
    }

    // The register is left justified. *DATA_H has bits 11:4 of the register. *DATA_L has bits 3:0.
    short rawX = readRegister(XDATA_H) << 8 | readRegister(XDATA_L); 
    short rawY = readRegister(YDATA_H) << 8 | readRegister(YDATA_L);
    short rawZ = readRegister(ZDATA_H) << 8 | readRegister(ZDATA_L);

    rawX = rawX >> 4; 
    rawY = rawY >> 4;
    rawZ = rawZ >> 4;

    // Converting raw axis data to acceleration in g unit
    x = rawX * SCALE_FACTOR * MG_TO_G;
    y = rawY * SCALE_FACTOR * MG_TO_G;
    z = rawZ * SCALE_FACTOR * MG_TO_G;
}

void ADXL372class::setOdr(Odr odr){
    byte odrShifted = odr << 5; // odr bits start from bit 5
    updateRegister(TIMING, odrShifted, ODR_MASK);
}

void ADXL372class::setWakeUpRate(WakeUpRate wur) {
    byte wurShifted = wur << 2; // wur bits start from bit 2
    updateRegister(TIMING, wurShifted, WAKEUP_RATE_MASK);
}

void ADXL372class::enableExternalClock(bool isEnabled){
    byte valueShifted = isEnabled << 1; // bit 1 in register
    updateRegister(TIMING, valueShifted, EXT_CLK_MASK);
}

void ADXL372class::enableExternalTrigger(bool isEnabled){
    updateRegister(TIMING, isEnabled, EXT_SYNC_MASK);
}

void ADXL372class::setBandwidth(Bandwidth bandwidth){
    updateRegister(MEASURE, bandwidth, BANDWIDTH_MASK);
}

void ADXL372class::enableLowNoiseOperation(bool isEnabled){
    byte valueShifted = isEnabled << 3; // bit 3 in register
    updateRegister(MEASURE, valueShifted, LOW_NOISE_MASK);
}

void ADXL372class::setLinkLoopActivityProcessing(LinkLoop activityProcessing){
    byte valueShifted = activityProcessing << 2; // bit 2 in register
    updateRegister(MEASURE, valueShifted, LINKLOOP_MASK);
}

void ADXL372class::enableAutosleep(bool isEnabled){
    byte valueShifted = isEnabled << 6; // bit 6 in register
    updateRegister(MEASURE, valueShifted, AUTOSLEEP_MASK);
}

void ADXL372class::setOperatingMode(OperatingMode opMode){
    updateRegister(POWER_CTL, opMode, MODE_MASK);
}

void ADXL372class::disableHighPassFilter(bool isDisabled){
    byte valueShifted = isDisabled << 2; // bit 2 in register
    updateRegister(POWER_CTL, valueShifted, HPF_DISABLE_MASK);
}

void ADXL372class::disableLowPassFilter(bool isDisabled){
    byte valueShifted = isDisabled << 3; // bit 3 in register
    updateRegister(POWER_CTL, valueShifted, LPF_DISABLE_MASK);
}

void ADXL372class::setFilterSettling(FilterSettlingPeriod filterSettling){
    byte valueShifted = filterSettling << 4; // bit 4 in register
    updateRegister(POWER_CTL, valueShifted, LPF_DISABLE_MASK);
}

void ADXL372class::setInstantOnThreshold(InstantOnThreshold threshold){
    byte valueShifted = threshold << 5; // bit 5 in register
    updateRegister(POWER_CTL, valueShifted, LPF_DISABLE_MASK);
}

uint8_t ADXL372class::readRegister(byte regAddress){
    digitalWrite(m_csPin, LOW);
    regAddress = regAddress << 1 | 1; // Reading from a register
    SPI.transfer(regAddress);
    uint8_t value = SPI.transfer(0x00); // Transfering dummy byte to recieve data from accelerometer
    digitalWrite(m_csPin, HIGH);
    return value;
}

void ADXL372class::writeRegister(byte regAddress, uint8_t value) {
    digitalWrite(m_csPin, LOW); 
    regAddress = regAddress << 1; // Writing to a register
    SPI.transfer(regAddress);
    SPI.transfer(value);
    digitalWrite(m_csPin, HIGH);
}

void ADXL372class::updateRegister(byte regAddress, uint8_t value, byte mask) {
    // Need to use bitmasks to only change the desired bits in the registers
    byte registerState = readRegister(regAddress);
    registerState &= mask;
    value |= registerState & ~mask;
    writeRegister(regAddress, value);
}

