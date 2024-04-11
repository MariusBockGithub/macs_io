#include "macs_io.h"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <bit>
#include <QDebug>
#include <QCoreApplication>

enum ColorMode{Gray, GRBG, GBRG, BGGR, RGGB};

int main(int argc, char*argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();

    ColorMode colorMode = Gray;
    for (int i = 0; i<args.size(); i++)
    if (args[i].startsWith("color="))
    {
        QString color = args[i].mid(6).toUpper();
        args.removeAt(i);

        if (color=="gray")
            colorMode = Gray;
        else if (color=="RGGB")
            colorMode = RGGB;
        else if (color=="GRBG")
            colorMode = GRBG;
        else if (color=="GBRG")
            colorMode = GBRG;
        else if (color=="BGGR")
            colorMode = BGGR;
        break;
    }
    if (args.size() != 3)
    {
        qDebug() << "Usage:\n\tmacs_encode [color=gray|GRBG|GBRG|BGGR|RGGB] in.jpg out.macs";
        return -1;
    }
    cv::Mat in = cv::imread(args[1].toStdString(), cv::IMREAD_ANYDEPTH|cv::IMREAD_ANYCOLOR);
    if (in.empty())
    {
        qDebug() << "Error loading" << args[1];
        return -1;
    }
    if (in.channels() != 1 || in.depth() != CV_8U && in.depth() != CV_16U)
    {
        qDebug() << "Unsupported image format" << args[1];
        return -1;
    }

    // The native Macs format is GRBG. We crop the image if necessary to achieve this.
    switch (colorMode)
    {
    case Gray:
    case GRBG:
        break;
        
    case GBRG:
        in = in(cv::Rect(1, 1, in.cols-2, in.rows-2));
        break;

    case BGGR:
        in = in(cv::Rect(0, 1, in.cols, in.rows-2));
        break;

    case RGGB:
        in = in(cv::Rect(1, 0, in.cols-2, in.rows));
        break;

    }
    
    // Add some fake meta data:
    MACS::MacsMetaData meta;
    meta.camVendor = "DLR-OS";
    meta.camModel = "hr16070MFLGEC";
    meta.camName = "Cam-NIR-50";
    meta.camSerial = "33552";
    meta.camMAC = "ac:4f:fc:00:51:f4";
    meta.camIP = "192.168.101.236";
    meta.camFirmware = "1.6.5";
    meta.imageID = 2135;
    meta.imageIDX = 2135;
    meta.tapCount = 4;
    meta.expTimeUS = 600;
    meta.timeStamp = 12466170; // georef.time.time().msecsSinceStartOfDay();

    // Optional: Add some fake geo reference data:
    MACS::PoseEvent georef;
    georef.time = QDateTime::currentDateTime();
    georef.roll = 0.87579313184499341;
    georef.pitch = 2.5735878935747571 ;
    georef.yaw = 23.812077442224226;
    georef.lat = 69.517744664608387;
    georef.lon = -139.09295924255881;
    georef.alt = 391.84334826655686;
    georef.veln = 57.103192015934745;
    georef.vele = 28.053951739469827;
    georef.velup = -0.17086418721157415;
    
    MACS::MacsImage out;

    out.georef = georef;
    out.meta = meta;


    QByteArray outData;
    outData.resize(in.rows*in.cols*sizeof(uint16_t));
    cv::Mat dst = cv::Mat(in.rows, in.cols, CV_16UC1, outData.data(), in.cols*sizeof(uint16_t));

    if (in.depth() == CV_8U)
    {
        for (int y = 0; y < in.rows; y++)
            for (int x = 0; x < in.cols; x++)
                dst.at<uint16_t>(y, x) = in.at<uint8_t>(y, x) << 8;
    }
    else
    {
        for (int y = 0; y< in.rows; y++)
            for (int x = 0; x < in.cols; x++)
                dst.at<uint16_t>(y, x) = in.at<uint16_t>(y, x);
    }
    auto outFormat = colorMode == Gray ? MACS::PixelFormat::Mono16 : MACS::PixelFormat::BayerGR16;;
    auto outEndianness = std::endian::native == std::endian::big ? MACS::PixelEndianness::Big : MACS::PixelEndianness::Little;
    out.imageData = MACS::MacsImageData(outData, in.cols, in.rows, in.cols, outFormat, outEndianness);

    // Set some options:
    bool preview = true;
    bool compression = true;

    // Write the image:
    out.save(args[2], true, true);
    return 0;
}