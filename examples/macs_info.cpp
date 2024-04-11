#include "macs_io.h"

int main(int argc, const char*argv[])
{
    if (argc != 2)
    {
        qDebug() << "Usage:\n\tmacs_info <macs-filename>";
        return -1;
    }
    MACS::MacsImage image;
    if (!image.load(argv[1]))
    {
        return -1;
    }
    qDebug() << "MACS Image info: " ;
    qDebug() << "  File name: " << QFileInfo(image.fileName).fileName();
    qDebug() << "  Dimensions: " << image.imageData.width() << "x" << image.imageData.height();
    qDebug() << "  Format: " << MACS::pixelformatToString(image.imageData.format());
    if (image.georef.isValid())
    {
        qDebug() << "  Pose event from" << image.georef.time.toString("yyyy-MM-dd hh.m.ss.zzz");

        qDebug() << "    Lat:" << image.georef.lat;
        qDebug() << "    Lon:" << image.georef.lon;
        qDebug() << "    Alt:" << image.georef.alt;
        
        qDebug() << "    Roll:" << image.georef.roll << "degrees.";
        qDebug() << "    Pitch:" << image.georef.pitch << "degrees.";
        qDebug() << "    Yaw:" << image.georef.yaw << "degrees.";

        double vel_mps = sqrt(
            image.georef.vele*image.georef.vele
          + image.georef.veln*image.georef.veln
          + image.georef.velup*image.georef.velup);
        
        qDebug() << "    Vel:" << vel_mps*3.6 << "km/h";
    }
    qDebug() << "\nMeta Data: " ;
    qDebug() << "  camVendor"<< image.meta.camVendor;
    qDebug() << "  camModel"<< image.meta.camModel;
    qDebug() << "  camName"<< image.meta.camName;
    qDebug() << "  camSerial"<< image.meta.camSerial;
    qDebug() << "  camMAC"<< image.meta.camMAC;
    qDebug() << "  camIP"<< image.meta.camIP;
    qDebug() << "  camFirmware"<< image.meta.camFirmware;
    qDebug() << "  affix"<< image.meta.affix;
    return 0;
}