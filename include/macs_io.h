#pragma once
#include <QDateTime>
#include <QSize>
#include <QDebug>
#include <QFileInfo>
#include <opencv2/core.hpp>
#include <QImage>

namespace MACS
{
namespace Log
{
    enum Level{
        Warning,
        Error
    };
}  // namespace Log

#ifdef NOLOG
struct NoOp
{
    template<typename T>
    NoOp& operator <<(const T&)
    {
        return *this;
    }
};
static NoOp log(Log::Level)
{
    return {};
}
#else
static QDebug log(Log::Level)
{
    return qWarning();
}
#endif

    enum class PixelFormat : std::uint32_t
    {
        Mono12Packed = 0x010C0047,
        BayerGR12Packed = 0x010C0057,
        BayerBG12Packed = 0x010C0053,
        BayerGB12Packed = 0x010C002C,
        BayerRG12Packed = 0x010C002B,
        Mono16 = 0x01100007,
        BayerGR16 = 0x0110002E,
        BayerBG16 = 0x01100031,
        BayerRG16 = 0x0110002F,
        BayerGB16 = 0x01100030,
        INVALID = 0
    };

    QString pixelformatToString(PixelFormat format);

    enum class PixelEndianness : std::uint32_t
    {
        Undefined = 0,
        Big = 1,
        Little = 2
    };

    struct PoseEvent
    {
        QDateTime time;
        double roll = 0, pitch = 0, yaw = 0;
        double lat = 0, lon = 0, alt = 0;
        double veln = 0, vele = 0, velup = 0; // Velocity north east up m/s

        bool isValid() const { return time.isValid(); }
    };

    QDataStream& operator>>(QDataStream& in, PoseEvent& poseEvent);

    QDataStream& operator<<(QDataStream& out, const PoseEvent& poseEvent);

    struct CorrectionOptions
    {
        struct Stretch
        {
            double gamma = 1.0;
            double min = 0.0, max = 1.0;
        } stretch;

        struct Devignetting
        {
            // Formula (12) in http://www.stanford.edu/~rohlfing/2012-rohlfing-single_image_vignetting_correction.pdf
            uint16_t offset = 0;
            double factor = 1.0;
            double a = 0.0, b = 0.0, c = 0.0;
            double cx = 0.0, cy = 0.0;
        } devignetting;

        struct ColorBalance
        {
            double r = 1.0, g = 1.0, b = 1.0;
        } color_balance;

        struct Distortion
        {
            double cx_px = -1.0, cy_px = -1.0;
            double k1 = 0.0, k2 = 0.0, k3 = 0.0;
        } distortion;

        bool convertTo8bit = true;
    };

    enum class MacsImageFormatVersion : qint32
    {
        Version_1 = 1,
        Version_2 = 2
    };

    struct MacsCameraInfo
    {
        QByteArray vendor;
        QByteArray model;
        QByteArray name;
        QByteArray serial;
        QByteArray mac;
        QByteArray ip;
        QByteArray firmware;

        MacsCameraInfo() {}

        MacsCameraInfo(
            const QByteArray v, const QByteArray mo, const QByteArray n, const QByteArray s, const QByteArray ma, const QByteArray i, const QByteArray f)
            : vendor(v)
            , model(mo)
            , name(n)
            , serial(s)
            , mac(ma)
            , ip(i)
            , firmware(f)
        {}
    };

    struct MacsPhotoInfo
    { // this is just a conveniance struct to simplify the handling of (dynamic) imageData information
        qint32 imageID = -1;
        qint32 imageIDX = -1;
        qint32 tapCount = -1;
        qint32 expTimeUS = -1;
        qint32 timeStamp = -1;
        QByteArray affix;

        MacsPhotoInfo() {}
        MacsPhotoInfo(qint32 id, qint32 idx, qint32 tc, qint32 et, qint32 ts, QByteArray a)
            : imageID(id)
            , imageIDX(idx)
            , tapCount(tc)
            , expTimeUS(et)
            , timeStamp(ts)
            , affix(a)
        {}
    };

    class MacsMetaData
    {
    public:
        QByteArray camVendor;
        QByteArray camModel;
        QByteArray camName;
        QByteArray camSerial;
        QByteArray camMAC;
        QByteArray camIP;
        QByteArray camFirmware;

        qint32 imageID = -1;
        qint32 imageIDX = -1;
        qint32 tapCount = -1;
        qint32 expTimeUS = -1;
        qint32 timeStamp = -1;
        QByteArray affix;

        QByteArray comment;

        MacsMetaData() {}
        MacsMetaData(const MacsCameraInfo& mci, const MacsPhotoInfo& mpi, const QByteArray& note);
        MacsMetaData(const QByteArray& data);
    };

    QDataStream& operator<<(QDataStream& out, const MacsMetaData& meta);

    QDataStream& operator>>(QDataStream& in, MacsMetaData& meta);

    enum class CopyPolicy{ DEEP_COPY, SHALLOW_COPY };

    class MacsImageData
    {
    public:
        MacsImageData();
        // - pitch is number of bytes per line
        MacsImageData(QByteArray buffer, int pixelWidth, int pixelHeight, int pixelPitch, PixelFormat format, PixelEndianness endianness);
        MacsImageData(const char* buffer,
                      int bufferByteSize,
                      int pixelWidth,
                      int pixelHeight,
                      int pixelPitch,
                      PixelFormat format,
                      PixelEndianness endianness,
                      CopyPolicy copyPolicy = CopyPolicy::DEEP_COPY);

        // - rawData as it was stored
        // - it might be packed
        // - take the endianness into account
        // - do not change it inplace unless you also adjust the format and endianness
        const QByteArray& rawData() const { return imageData; }
        QByteArray& rawData() { return imageData; }

        // - this method is expensive to calculate!
        // - unpacks the data
        // - converts it to little endian 16 bit
        // - if the data was 12 Bit it will be shifted by 4 Bits
        QByteArray data() const;

        int width() const { return imageWidth; }
        int height() const { return imageHeight; }
        // - pitch in pixels
        int pitch() const { return imagePitch; }
        int byteSize() const { return imageData.size(); }
        PixelFormat format() const { return interpretation; }
        PixelEndianness endianness() const { return pixelEndianness; }

        bool isValid() const;
        bool isMono() const;
        bool isColor() const;
        int bitDepth() const;

    private:
        QByteArray imageData;
        std::int32_t imageWidth;
        std::int32_t imageHeight;
        std::int32_t imagePitch;
        PixelFormat interpretation;
        PixelEndianness pixelEndianness = PixelEndianness::Undefined;
    };

    class MacsImage
    {
    public:
        bool load(const QString& fileName);

        bool save(const QString& fileName, bool preview = false, bool compression = false) const;

        bool isValid() const;

        // - returns a 16 bit image
        // - it has 1 or 3 channel
        cv::Mat getCorrectedImage(const CorrectionOptions& options = {}) const;

        QImage to8Bit(const CorrectionOptions& options = {}) const;

        bool exportImage(const QString& fileName, const CorrectionOptions& options = {}) const;

        QString fileName;
        // - contains the image data and properties like pixel-format and size
        MacsImageData imageData;
        // - contains meta data like timestamp or sensor name
        MacsMetaData meta;
        // - contains the geo reference of the image
        PoseEvent georef;
    };

} // namespace MACS
