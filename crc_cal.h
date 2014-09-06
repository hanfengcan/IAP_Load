#ifndef CRC_CAL_H
#define CRC_CAL_H

#include <QByteArray>

class CRC_Cal
{
private:
    unsigned long crc_val;

public:
    CRC_Cal();

    void crc32(const QByteArray *data, unsigned long len);
    unsigned long getCrc32();
};

#endif // CRC_CAL_H
