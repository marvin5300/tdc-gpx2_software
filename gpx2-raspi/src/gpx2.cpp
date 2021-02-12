#include "gpx2.h"
#include <iostream>

extern "C" {
#include <pigpiod_if2.h>
}

GPX2::GPX2() {
    pi = pigpio_start((char*)"127.0.0.1", (char*)"8888");
    if (pi < 0) {
        std::cerr << "Could not connect to pigpio daemon. Is pigpiod running? Start with sudo pigpiod";
        return;
    }
    spiInitialise();
}

GPX2::~GPX2() {
    if (spiHandle >= 0) {
        bool status = spi_close(pi, spiHandle);
        if (status != 0) {
            std::cerr << "pi bad handle";
        }
    }
    if (pi >= 0) {
        bool status = pigpio_stop(pi);
        if (status != 0) {
            std::cerr << "pi bad handle";
        }
    }
}

bool GPX2::writeSpi(uint8_t command, std::string data) {
    if (!spiInitialised) {
        if (!spiInitialise()) {
            return;
        }
    }
    std::string tx = ((char)command) + data;
    char *txBuf = (char*)malloc(data.size() + 1);
    txBuf[0] = (char)command;
    for (unsigned int i = 1; i < data.size() + 1; i++) {
        txBuf[i] = data[i - 1];
    }
    char *rxBuf = (char*)malloc(data.size() + 1);
    bool status = spi_xfer(pi, spiHandle, txBuf, rxBuf, data.size() + 1) == 1 + data.size();
    if (!status) {
        std::cerr << "writeSpi(uint8_t, std::string): wrong number of bytes transfered";
    }

    free(txBuf);
    free(rxBuf);
    return status;
}

std::string GPX2::readSpi(uint8_t command, unsigned int bytesToRead) {
    if (!spiInitialised) {
        if (!spiInitialise()) {
            return;
        }
    }
    //char buf[bytesToRead];
    //char charCommand = command;
    //spi_write(pi, spiHandle, &charCommand, 1);
    //spi_read(pi, spiHandle, buf, bytesToRead);

    char *rxBuf = (char*)malloc(bytesToRead + 1);
    char *txBuf = (char*)malloc(bytesToRead + 1);
    txBuf[0] = (char)command;
    for (unsigned int i = 1; i < bytesToRead; i++) {
        txBuf[i] = 0;
    }
    bool status = spi_xfer(pi, spiHandle, txBuf, rxBuf, bytesToRead + 1) == 1 + bytesToRead;
    if (!status){
        std::cerr << "readSpi(uint8_t, unsigned int): wrong number of bytes transfered";
        return "";
    }

    std::string data;
    for (unsigned int i = 1; i < bytesToRead + 1; i++) {
        data += rxBuf[i];
    }
    free(txBuf);
    free(rxBuf);
    return data;
}

bool GPX2::isSpiInitialised(){
    return spiInitialised;
}

bool GPX2::spiInitialise(){
    if (!spiInitialised){
        std::cerr << "pigpiohandler not initialised";
        return false;
    }
    if (spiInitialised){
        return true;
    }
    spiHandle = spi_open(pi, 0, spi_freq, spi_flags);
    if (spiHandle<0){
        std::string errstr="";
        switch (spiHandle){
        case PI_BAD_CHANNEL:
            errstr="DMA channel not 0-15";
            break;
        case PI_BAD_SPI_SPEED:
            errstr="bad SPI speed";
            break;
        case PI_BAD_FLAGS:
            errstr="bad spi open flags";
            break;
        case PI_NO_AUX_SPI:
            errstr="no auxiliary SPI on Pi A or B";
            break;
        case PI_SPI_OPEN_FAILED:
            errstr="can't open SPI device";
            break;
        default:
            break;
        }
        std::cerr<<"Could not initialise SPI bus:"<<errstr;
        return false;
    }
    spiInitialised = true;
    return true;
}