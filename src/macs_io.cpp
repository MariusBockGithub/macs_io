#include "macs_io.h"
#include <QBuffer>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

namespace MACS
{

    QDataStream& operator>>(QDataStream& in, PoseEvent& pe)
    {
        // Date-time:
        in >> pe.time;
        pe.time.setTimeSpec(Qt::UTC);

        // Position: First read the encoding. We only support reading 'type=1' - latlon:
        uint8_t type, number, letter;
        in >> type >> number >> letter;
        if (type == 2)
        {
            double x, y, z;
            in >> x >> y >> z;
            log(Log::Warning) << QString("Geo coordinates are encoded as UTM%1%2. They will be ignored").arg(number).arg(QChar(letter));
        }
        else if (type != 1 || number != 0 || letter != 0)
        {
            in.setStatus(QDataStream::ReadCorruptData);
            return in;
        }
        else
            in >> pe.lat >> pe.lon >> pe.alt;

        // Orientation:
        in >> pe.roll >> pe.pitch >> pe.yaw;

        // Velocity:
        in >> pe.veln >> pe.vele >> pe.velup;
        return in;
    }

    QDataStream& operator<<(QDataStream& out, const PoseEvent& pe)
    {
        // Date-time:
        out << pe.time;

        // Position: We write 'type=1': latlon:
        uint8_t type = 1, dummy = 0;
        out << type << dummy << dummy;
        out << pe.lat << pe.lon << pe.alt;

        // Orientation:
        out << pe.roll << pe.pitch << pe.yaw;

        // Velocity:
        out << pe.veln << pe.vele << pe.velup;
        return out;
    }

    QString pixelformatToString(PixelFormat format)
    {
        switch (format)
        {
        case PixelFormat::Mono12Packed:
            return "Mono12Packed";
        case PixelFormat::BayerGR12Packed:
            return "BayerGR12Packed";
        case PixelFormat::BayerBG12Packed:
            return "BayerBG12Packed";
        case PixelFormat::BayerGB12Packed:
            return "BayerGB12Packed";
        case PixelFormat::BayerRG12Packed:
            return "BayerRG12Packed";
        case PixelFormat::Mono16:
            return "Mono16";
        case PixelFormat::BayerGR16:
            return "BayerGR16";
        case PixelFormat::BayerBG16:
            return "BayerBG16";
        case PixelFormat::BayerRG16:
            return "BayerRG16";
        case PixelFormat::BayerGB16:
            return "BayerGB16";
        }
        return "INVALID";
    }

    namespace
    {
        const qint32 INITIAL_OFFSET = 17 * sizeof(qint32);
        const qint32 MAGIC_MACS_IMAGE_CONTAINER = ('M' << 24) + ('I' << 16) + ('C' << 8) + '#';
        const qint32 MAGIC_MACS_META = ('M' << 24) + ('M' << 16) + ('D' << 8) + '#';
        const qint32 MAGIC_MACS_GEO = ('G' << 24) + ('E' << 16) + ('O' << 8) + '#';
        const qint32 MAGIC_MACS_PREVIEW = ('M' << 24) + ('P' << 16) + ('V' << 8) + '#';
        const qint32 MAGIC_MACS_IMAGE_PIXEL_DATA = ('M' << 24) + ('I' << 16) + ('D' << 8) + '#';
        const qint32 MAGIC_OFFSET_META = 0x01000000;
        const qint32 MAGICS_OFFSET_GEO = 0x02000000;
        const qint32 MAGIC_OFFSET_PREVIEW = 0x03000000;
        const qint32 MAGICS_OFFSET_PIXEL_DATA = 0x04000000;

        struct MetaData
        {
            qint32 formatVersion = 0;
            qint32 offsetMeta = 0;
            qint32 offsetGeo = 0;
            qint32 offsetPreview = 0;
            qint32 offsetData = 0;
            quint32 imageSize = 0;
            quint32 imageWidth = 0;
            quint32 imageHeight = 0;
            quint32 imagePitch = 0;
            quint32 imageFormat = 0;
            quint32 imageCompression = 0;
            quint32 imageEndianness = 0;
        };

        struct ImageInfo
        {
            qint32 size = 0;
            qint32 width = 0;
            qint32 height = 0;
            qint32 pitch = 0;
            qint32 format = 0;
            qint32 compression = 0;
            qint32 endianness = 0;
        };

        bool readAllMetaDataFromQIODevice(QIODevice* device, MetaData* meta, std::string* error)
        {
            if (!device)
            {
                if (error)
                    *error = "Nullptr file handle provided.";
                return false;
            }
            device->seek(0);
            if (!meta)
            {
                if (error)
                    *error = "Nullptr meta handle provided.";
                return false;
            }
            QDataStream stream(device);
            stream.setVersion(QDataStream::Qt_5_4);

            qint32 magic = 0;
            stream >> magic;
            if (magic != MAGIC_MACS_IMAGE_CONTAINER)
            {
                if (error)
                    *error = "Missing MAGIC_MACS_IMAGE_CONTAINER value.";
                return false;
            }

            stream >> meta->formatVersion;
            qint32 magicMeta = 0;
            stream >> magicMeta >> meta->offsetMeta;
            if (magicMeta != MAGIC_OFFSET_META)
            {
                if (error)
                    *error = "Missing MAGIC_OFFSET_META value.";
                return false;
            }

            qint32 magicGeo = 0;
            stream >> magicGeo >> meta->offsetGeo;
            if (magicGeo != MAGICS_OFFSET_GEO)
            {
                if (error)
                    *error = "Missing MAGICS_OFFSET_GEO value.";
                return false;
            }

            qint32 magicPreview = 0;
            stream >> magicPreview >> meta->offsetPreview;
            if (magicPreview != MAGIC_OFFSET_PREVIEW)
            {
                if (error)
                    *error = "Missing MAGIC_OFFSET_PREVIEW value.";
                return false;
            }

            qint32 magicPixelData = 0;
            stream >> magicPixelData >> meta->offsetData;
            if (magicPixelData != MAGICS_OFFSET_PIXEL_DATA)
            {
                if (error)
                    *error = "Missing MAGICS_OFFSET_PIXEL_DATA value.";
                return false;
            }

            stream >> meta->imageSize >> meta->imageWidth >> meta->imageHeight >> meta->imagePitch >> meta->imageFormat >> meta->imageCompression;
            if (static_cast<MacsImageFormatVersion>(meta->formatVersion) == MacsImageFormatVersion::Version_1)
            {
                switch (meta->imageFormat)
                {
                // - only SVS cameras had big endian while packed
                case (quint32) PixelFormat::BayerGR12Packed:
                case (quint32) PixelFormat::Mono12Packed:
                    meta->imageEndianness = static_cast<quint32>(PixelEndianness::Big);
                    break;
                // - XIMEA and TIR cameras were little endian
                // - all unpacked data was little endian
                default:
                    meta->imageEndianness = static_cast<quint32>(PixelEndianness::Little);
                }
            }
            else
            {
                stream >> meta->imageEndianness;
            }

            if (stream.status() != QDataStream::Status::Ok)
            {
                if (error)
                    *error = "Error reading imageData properties.";
                return false;
            }

            return stream.status() == QDataStream::Status::Ok;
        }

        bool writeRawToByteArray(QByteArray* array,
                                 const MacsMetaData& metaData,
                                 const PoseEvent& poseEvent,
                                 const QByteArray& jpgPreview,
                                 const ImageInfo& imageInfo,
                                 const QByteArray& imageData,
                                 std::string* error)
        {
            QByteArray baMeta;
            {
                QDataStream stream(&baMeta, QIODevice::WriteOnly);
                stream.setVersion(QDataStream::Qt_5_4);
                stream << metaData;
            }

            QByteArray baGeo;
            if (poseEvent.isValid())
            {
                QDataStream stream(&baGeo, QIODevice::WriteOnly);
                stream.setVersion(QDataStream::Qt_5_4);
                stream << poseEvent;
            }
            const quint32 offsetMeta = INITIAL_OFFSET;
            const qint32 offsetGeo = offsetMeta + sizeof(MAGIC_MACS_META) + sizeof(quint32) + baMeta.size();
            const qint32 offsetPreview = offsetGeo + sizeof(MAGIC_MACS_GEO) + sizeof(quint32) + baGeo.size();
            const qint32 offsetData = offsetPreview + sizeof(MAGIC_MACS_PREVIEW) + sizeof(quint32) + jpgPreview.size();

            QDataStream stream(array, QIODevice::WriteOnly);
            stream.setVersion(QDataStream::Qt_5_4);
            stream << static_cast<qint32>(MAGIC_MACS_IMAGE_CONTAINER) << static_cast<qint32>(MacsImageFormatVersion::Version_2);

            // table of content
            stream << static_cast<qint32>(MAGIC_OFFSET_META) << static_cast<qint32>(offsetMeta) << static_cast<qint32>(MAGICS_OFFSET_GEO)
                   << static_cast<qint32>(poseEvent.isValid() ? offsetGeo : 0) << static_cast<qint32>(MAGIC_OFFSET_PREVIEW)
                   << static_cast<qint32>(!jpgPreview.isEmpty() ? offsetPreview : 0) << static_cast<qint32>(MAGICS_OFFSET_PIXEL_DATA)
                   << static_cast<qint32>(offsetData);

            // imageData attributes
            stream << static_cast<quint32>(imageInfo.size) << static_cast<quint32>(imageInfo.width) << static_cast<quint32>(imageInfo.height)
                   << static_cast<quint32>(imageInfo.pitch) << static_cast<quint32>(imageInfo.format) << static_cast<quint32>(imageInfo.compression)
                   << static_cast<quint32>(imageInfo.endianness);

            // meta data
            stream << static_cast<qint32>(MAGIC_MACS_META) << static_cast<quint32>(baMeta.size());
            stream.writeRawData(baMeta.data(), baMeta.size());

            // geo ref
            stream << static_cast<qint32>(MAGIC_MACS_GEO) << static_cast<quint32>(baGeo.size());
            stream.writeRawData(baGeo.data(), baGeo.size());

            // preview
            stream << static_cast<qint32>(MAGIC_MACS_PREVIEW) << static_cast<quint32>(jpgPreview.size());
            stream.writeRawData(jpgPreview.data(), jpgPreview.size());

            // pixel data
            stream << static_cast<qint32>(MAGIC_MACS_IMAGE_PIXEL_DATA) << static_cast<quint32>(imageData.size());
            stream.writeRawData(imageData.data(), imageData.size());

            return stream.status() == QDataStream::Status::Ok;
        }

        bool readRawFromQIODevice(QIODevice* file,
                                  MacsMetaData* metaData,
                                  PoseEvent* poseEvent,
                                  QByteArray* jpgPreview,
                                  ImageInfo* imageInfo,
                                  QByteArray* imageData,
                                  std::string* error)
        {
            if (!file)
            {
                if (error)
                    *error = "Nullptr file handle provided.";
                return false;
            }
            file->seek(0);
            MetaData meta;
            if (!readAllMetaDataFromQIODevice(file, &meta, error))
                return false;

            QDataStream stream(file);
            stream.setVersion(QDataStream::Qt_5_4);

            qint32 magic = 0;
            quint32 size = 0;
            if (metaData && meta.offsetMeta != 0)
            {
                if (!file->seek(meta.offsetMeta))
                {
                    if (error)
                        *error = "Could not seek to MetaData.";
                    return false;
                }
                stream >> magic;
                if (magic != MAGIC_MACS_META)
                {
                    if (error)
                        *error = "Missing MAGIC_MACS_META value.";
                    return false;
                }
                stream >> size >> *metaData;
                if (stream.status() != QDataStream::Status::Ok)
                {
                    if (error)
                        *error = "Error reading MetaData.";
                    return false;
                }
            }

            if (poseEvent && meta.offsetGeo != 0)
            {
                if (!file->seek(meta.offsetGeo))
                {
                    if (error)
                        *error = "Could not seek to GeoPose.";
                    return false;
                }
                stream >> magic >> size >> *poseEvent;
                if (stream.status() != QDataStream::Status::Ok)
                {
                    if (error)
                        *error = "Error reading GeoPose.";
                    return false;
                }
            }

            if (jpgPreview && meta.offsetPreview != 0)
            {
                if (file->seek(meta.offsetPreview))
                {
                    stream >> magic >> size;
                    if (size > 0 && size < file->size())
                    {
                        jpgPreview->resize(size);
                        const auto ret = stream.readRawData(jpgPreview->data(), size);
                        if (ret != size || stream.status() != QDataStream::Status::Ok)
                        {
                            jpgPreview->clear();
                            if (error)
                                *error = "Error reading Preview.";
                        }
                    }
                    else if (error)
                        *error = "Preview byteSize error.";
                }
                else if (error)
                    *error = "Could not seek to Preview.";
            }

            if (imageInfo)
            {
                imageInfo->width = meta.imageWidth;
                imageInfo->height = meta.imageHeight;
                imageInfo->size = meta.imageSize;
                imageInfo->pitch = meta.imagePitch;
                imageInfo->format = meta.imageFormat;
                imageInfo->compression = meta.imageCompression;
                imageInfo->endianness = meta.imageEndianness;
            }

            if (imageData && meta.offsetData != 0)
            {
                if (!file->seek(meta.offsetData))
                {
                    if (error)
                        *error = "Could not seek to PixelData.";
                    return false;
                }
                stream >> magic >> size;
                imageData->resize(size);
                stream.readRawData(imageData->data(), size);
                if (stream.status() != QDataStream::Status::Ok)
                {
                    if (error)
                        *error = "Error reading PixelData.";
                    return false;
                }
            }
            return stream.status() == QDataStream::Status::Ok;
        }

        bool loadFromIODevice(QIODevice& device, MacsImage* macs, QString* errorString)
        {
            MacsMetaData meta;
            PoseEvent georef;
            ImageInfo imageInfo;
            QByteArray imageData;
            QByteArray jpgPreview;
            std::string error;
            if (!readRawFromQIODevice(&device, &meta, &georef, &jpgPreview, &imageInfo, &imageData, &error))
            {
                if (errorString)
                    *errorString = QString::fromStdString(error);
                return false;
            }
            macs->meta = meta;
            macs->georef = georef;
            if (imageInfo.compression)
                imageData = qUncompress(imageData);
            macs->imageData = MacsImageData(imageData,
                                            imageInfo.width,
                                            imageInfo.height,
                                            imageInfo.pitch,
                                            static_cast<PixelFormat>(imageInfo.format),
                                            static_cast<PixelEndianness>(imageInfo.endianness));
            return true;
        }

        template<typename T>
        T square(T v)
        {
            return v * v;
        }

    } // unnamed namespace

    bool MacsImage::load(const QString& fileName)
    {
        QFile file(fileName);
        if (!file.open(QFile::ReadOnly))
        {
            log(Log::Error) << "Could not open file" << fileName;
            return false;
        }
        QString errorString;
        if (!loadFromIODevice(file, this, &errorString))
        {
            log(Log::Error) << errorString;
            return false;
        }
        this->fileName = fileName;
        return true;
    }

    bool MacsImage::save(const QString& fileName, bool preview, bool compression) const
    {
        QByteArray baPreview;
        if (preview)
        {
            QImage qImage = to8Bit().scaledToWidth(256);
            QBuffer buffer(&baPreview);
            if (buffer.open(QIODevice::WriteOnly))
            {
                if (!qImage.save(&buffer, "JPG", 50))
                    log(Log::Error) << "Could not write preview to buffer.";
                buffer.close();
            }
        }

        QByteArray macsByteArray;
        ImageInfo imageInfo;
        QByteArray data;
        if (compression)
        {
            imageInfo.compression = 1;
            data = qCompress(imageData.rawData());
        }
        else
        {
            imageInfo.compression = 0;
            data = imageData.rawData();
        }
        imageInfo.width = imageData.width();
        imageInfo.height = imageData.height();
        imageInfo.pitch = imageData.pitch();
        imageInfo.format = static_cast<quint32>(imageData.format());
        imageInfo.endianness = static_cast<qint32>(imageData.endianness());
        imageInfo.size = imageData.byteSize();

        std::string error;
        if (!writeRawToByteArray(&macsByteArray, meta, georef, baPreview, imageInfo, data, &error))
        {
            log(Log::Error) << error;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly))
        {
            log(Log::Error) << QString("cannot open file '%1' for writing").arg(fileName);
            return 0;
        }

        qint64 written = file.write(macsByteArray);

        QFileDevice::FileError errorCode = file.error();
        if (errorCode != QFileDevice::NoError)
        {
            log(Log::Error) << QString("Error writing imageData file '%1' (error code: %2)").arg(fileName).arg(errorCode);
            return false;
        }
        file.close();
        return true;
    }

    bool MacsImage::isValid() const
    {
        return !imageData.rawData().isEmpty();
    }

    cv::Mat MacsImage::getCorrectedImage(const CorrectionOptions& options) const
    {
        if (!isValid())
            return {};
        cv::Mat img_16Bit;
        img_16Bit = cv::Mat(imageData.height(), imageData.width(), CV_16UC1, (void*) imageData.data().data(), imageData.pitch() * sizeof(std::uint16_t)).clone();
        // Apply model based devignetting.
        // Formula (12) in http://www.stanford.edu/~rohlfing/2012-rohlfing-single_image_vignetting_correction.pdf
        if (options.devignetting.a != 0.0 || options.devignetting.b != 0.0 || options.devignetting.c != 0.0)
        {
            const double a = options.devignetting.a;
            const double b = options.devignetting.b;
            const double c = options.devignetting.c;
            const double factor = options.devignetting.factor;
            const uint16_t offset = options.devignetting.offset;

            const double cx = options.devignetting.cx;
            const double cy = options.devignetting.cy;

            const int w = imageData.width();
            const int h = imageData.height();

            double R = sqrt(2 * square(qMax(w, h) / 2)); //<! The distance from the middle to a corner
            double cx_px = w / 2 + cx * (w / 2);         //<! Center in px
            double cy_px = h / 2 + cy * (h / 2);         //<! Center in px

            for (int y = 0; y < imageData.height(); y++)
                for (int x = 0; x < imageData.width(); x++)
                {
                    uint16_t src = img_16Bit.at<uint16_t>(y, x);

                    double u = x - cx_px;               //<! Distance from center in px
                    double v = y - cy_px;               //<! Distance from center in px
                    double r = sqrt(u * u + v * v) / R; //<! Normalized distance from center
                    double dst = (src - offset) / (1.0 + a * r * r + b * r * r * r * r + c * r * r * r * r * r * r) * factor;

                    img_16Bit.at<uint16_t>(y, x) = qBound<int>(0, dst, 65535);
                }
        }

        // Stretch and apply gamma
        if (options.stretch.min != 0.0 || options.stretch.max != 1.0 || options.stretch.gamma != 1.0)
        {
            double min = qBound(0.0, options.stretch.min, 1.0);
            double max = qBound(min, options.stretch.max, 1.0);
            const double srcMin = min * 65535, srcMax = max * 65535;
            const double dstMin = 0, dstMax = 65535;

            const double alpha = (dstMax - dstMin) / (srcMax - srcMin);
            const double beta = dstMin - srcMin * alpha;

            for (int y = 0; y < imageData.height(); y++)
                for (int x = 0; x < imageData.width(); x++)
                {
                    uint16_t src = img_16Bit.at<uint16_t>(y, x);
                    double p = (src - srcMin) / (srcMax - srcMin);
                    p = pow(p, options.stretch.gamma);
                    double dst = dstMin + p * (dstMax - dstMin);
                    img_16Bit.at<uint16_t>(y, x) = qBound<int>(0, dst, 65535);
                }
        }

        // Apply color balance
        if (options.color_balance.r != 1.0 || options.color_balance.g != 1.0 || options.color_balance.b != 1.0)
        {
            for (int y = 0; y < imageData.height() - 1; y += 2)
            {
                for (int x = 0; x < imageData.width() - 1; x += 2)
                {
                    img_16Bit.at<uint16_t>(y, x) = qBound<int>(0, options.color_balance.g * img_16Bit.at<uint16_t>(y, x), 65535);
                    img_16Bit.at<uint16_t>(y, x + 1) = qBound<int>(0, options.color_balance.r * img_16Bit.at<uint16_t>(y, x + 1), 65535);
                }
                for (int x = 0; x < imageData.width() - 1; x += 2)
                {
                    img_16Bit.at<uint16_t>(y + 1, x) = qBound<int>(0, options.color_balance.b * img_16Bit.at<uint16_t>(y + 1, x), 65535);
                    img_16Bit.at<uint16_t>(y + 1, x + 1) = qBound<int>(0, options.color_balance.g * img_16Bit.at<uint16_t>(y + 1, x + 1), 65535);
                }
            }
        }
        // Debayer.
        switch (imageData.format())
        {
        case PixelFormat::BayerGR16:
        case PixelFormat::BayerGR12Packed:
            cv::cvtColor(img_16Bit, img_16Bit, cv::COLOR_BayerGB2RGB);
            break;
        case PixelFormat::BayerBG16:
        case PixelFormat::BayerBG12Packed:
            cv::cvtColor(img_16Bit, img_16Bit, cv::COLOR_BayerRG2RGB);
            break;
        case PixelFormat::BayerGB16:
        case PixelFormat::BayerGB12Packed:
            cv::cvtColor(img_16Bit, img_16Bit, cv::COLOR_BayerGR2RGB);
            break;
        case PixelFormat::BayerRG16:
        case PixelFormat::BayerRG12Packed:
            cv::cvtColor(img_16Bit, img_16Bit, cv::COLOR_BayerBG2RGB);
            break;
        default:
            break;
        }

        // Undistort
        if (options.distortion.k1 != 0 || options.distortion.k2 != 0.0 || options.distortion.k3 != 0.0)
        {
            const double w = img_16Bit.cols;
            const double h = img_16Bit.rows;
            const double cx_px = options.distortion.cx_px;
            const double cy_px = options.distortion.cy_px;
            const double k1 = options.distortion.k1;
            const double k2 = options.distortion.k2;
            const double k3 = options.distortion.k3;

            // This functor corrects radial lens distortion.
            auto undistort = [&](double x_in, double y_in, double& x_out, double& y_out) {
                // Subtract principal point. (Calculate normalized pixels.)
                const double u_ = x_in - cx_px;
                const double v_ = y_in - cy_px;

                // Calculate radial distortion.
                const double r2 = u_ * u_ + v_ * v_, r4 = r2 * r2, r6 = r2 * r4;
                const double radXY = 1.0 + k1 * r2 + k2 * r4 + k3 * r6;

                // Correct normalized pixels.
                const double u = u_ * radXY;
                const double v = v_ * radXY;

                // Add principal point.
                x_out = u + cx_px;
                y_out = v + cy_px;
            };

            // warp the imageData using cv::remap
            cv::Mat map(h, w, CV_32FC2);
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                {
                    double x_out, y_out;
                    undistort(x, y, x_out, y_out);
                    map.at<std::complex<float>>(y, x) = std::complex<float>((float) x_out, (float) y_out);
                }
            cv::Mat map1, map2;
            cv::convertMaps(map, cv::Mat(), map1, map2, CV_16SC2);
            cv::remap(img_16Bit, img_16Bit, map1, map2, cv::INTER_CUBIC, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
        }

        if (options.convertTo8bit)
        {
            cv::Mat img_8bit;
            img_16Bit.convertTo(img_8bit, CV_MAKETYPE(CV_8U, img_16Bit.channels()), 1. / 255);
            return img_8bit;
        }
        return img_16Bit;
    }

    QImage MacsImage::to8Bit(const CorrectionOptions& options) const
    {
        const cv::Mat img = getCorrectedImage(options);
        if (img.empty() || img.depth() != CV_16U || img.channels() > 3)
        {
            return {};
        }

        QImage qimage(QSize(imageData.width(), imageData.height()), img.channels() == 1 ? QImage::Format_Grayscale8 : QImage::Format_RGB888);

        cv::Mat wrapper(qimage.height(), qimage.width(), CV_MAKETYPE(CV_8U, img.channels()), qimage.bits(), qimage.bytesPerLine());
        img.convertTo(wrapper, wrapper.type(), 1. / 255.0);

        return qimage;
    }

    bool MacsImage::exportImage(const QString& fileName, const CorrectionOptions& options) const
    {
        if (options.convertTo8bit)
            return to8Bit(options).save(fileName);
        const cv::Mat mat(getCorrectedImage(options));
        // - cv::imwrite expects the imageData to be stored in 'BGR' order
        if (mat.channels() == 3)
            cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
        return cv::imwrite(fileName.toStdString(), mat);
    }

    namespace
    {

        // - requires 3 bytes that contain two 12 bit packed integers in little endian format
        // - shifts the data, so that the smallest 4 bits are 0
        // - the bit shift is required for later use in the MACS stack
        inline void unpack12BitLittleEndianAndShift(const std::uint8_t* src, std::uint8_t* dest1, std::uint8_t* dest2)
        {
            // pixel 1
            dest1[0] = src[0];
            dest1[1] = src[1] & 0x0F;
            auto ptr1 = reinterpret_cast<std::uint16_t*>(dest1);
            *ptr1 <<= 4;
            // pixel 2
            dest2[0] = src[1] & 0xF0;
            dest2[1] = src[2];
        }

        inline void unpack12BitBigEndianAndShift(const std::uint8_t* src, std::uint8_t* dest1, std::uint8_t* dest2)
        {
            // pixel 1
            dest1[0] = (src[1] & 0x0F) << 4;
            dest1[1] = src[0];
            // pixel 2
            dest2[0] = src[1] & 0xF0;
            dest2[1] = src[2];
        }

    } // namespace

    MacsImageData::MacsImageData()
        : imageData()
        , imageWidth(0)
        , imageHeight(0)
        , imagePitch(0)
        , interpretation(PixelFormat::INVALID)
    {}

    MacsImageData::MacsImageData(QByteArray buffer, int pixelWidth, int pixelHeight, int pixelPitch, PixelFormat format, PixelEndianness endianness)
        : imageData(buffer)
        , imageWidth(pixelWidth)
        , imageHeight(pixelHeight)
        , imagePitch(pixelPitch)
        , interpretation(format)
        , pixelEndianness(endianness)
    {}

    MacsImageData::MacsImageData(
            const char* buffer, int bufferByteSize, int pixelWidth, int pixelHeight, int pixelPitch, PixelFormat format, PixelEndianness endianness, CopyPolicy copyPolicy)
        : imageData(copyPolicy == CopyPolicy::SHALLOW_COPY ? QByteArray::fromRawData(buffer, bufferByteSize) // makes a shallow copy of buffer!
                                                           : QByteArray(buffer, bufferByteSize))             // makes a deep copy of buffer!
        , imageWidth(pixelWidth)
        , imageHeight(pixelHeight)
        , imagePitch(pixelPitch)
        , interpretation(format)
        , pixelEndianness(endianness)
    {}

    bool MacsImageData::isValid() const
    {
        return (!imageData.isEmpty()) && (interpretation != PixelFormat::INVALID);
    }

    bool MacsImageData::isMono() const
    {
        return interpretation == PixelFormat::Mono12Packed || interpretation == PixelFormat::Mono16;
    }

    bool MacsImageData::isColor() const
    {
        return interpretation == PixelFormat::BayerGR12Packed || interpretation == PixelFormat::BayerBG12Packed
               || interpretation == PixelFormat::BayerGB12Packed || interpretation == PixelFormat::BayerRG12Packed || interpretation == PixelFormat::BayerGR16
               || interpretation == PixelFormat::BayerBG16 || interpretation == PixelFormat::BayerGB16 || interpretation == PixelFormat::BayerRG16;
    }

    int MacsImageData::bitDepth() const
    {
        switch (interpretation)
        {
        case PixelFormat::INVALID:
            return 0;
        case PixelFormat::BayerGR12Packed:
        case PixelFormat::BayerBG12Packed:
        case PixelFormat::BayerGB12Packed:
        case PixelFormat::BayerRG12Packed:
            return 12;
        }
        return 16;
    }

    QByteArray MacsImageData::data() const
    {
        QByteArray b16Array;
        if (imageData.isEmpty())
            return b16Array;
        if (endianness() == PixelEndianness::Big)
        {
            switch (interpretation)
            {
            case PixelFormat::BayerGR12Packed:
            case PixelFormat::BayerBG12Packed:
            case PixelFormat::BayerGB12Packed:
            case PixelFormat::BayerRG12Packed:
            case PixelFormat::Mono12Packed: {
                b16Array.resize(imageWidth * imageHeight * 2);
                const quint8* src = reinterpret_cast<const quint8*>(imageData.constData());
                quint8* dst = reinterpret_cast<quint8*>(b16Array.data());
                for (int i = 0; i < imageWidth * imageHeight / 2; ++i)
                {
                    unpack12BitBigEndianAndShift(src, dst, &dst[2]);
                    // - two 16 bit ints = 4 bytes
                    std::advance(dst, 4);
                    // - two 12 bit ints = 3 bytes when packed
                    std::advance(src, 3);
                }
            }
            break;
            case PixelFormat::BayerGR16:
            case PixelFormat::BayerBG16:
            case PixelFormat::BayerGB16:
            case PixelFormat::BayerRG16:
            case PixelFormat::Mono16: {
                b16Array = imageData;
                quint8* byte1 = reinterpret_cast<quint8*>(b16Array.data());
                quint8* byte2 = byte1;
                std::advance(byte2, 1);
                for (int i = 0; i < imageWidth * imageHeight; ++i)
                {
                    std::swap(*byte1, *byte2);
                    std::advance(byte1, 2);
                    std::advance(byte2, 2);
                }
            }
            break;
            default:
                qCritical() << "Unknown PixelFormat in MacsImage::data16unpacked - returning empty ByteArray";
            }
        }
        else if (endianness() == PixelEndianness::Little)
        {
            switch (interpretation)
            {
            case PixelFormat::BayerGR12Packed:
            case PixelFormat::BayerBG12Packed:
            case PixelFormat::BayerGB12Packed:
            case PixelFormat::BayerRG12Packed:
            case PixelFormat::Mono12Packed: {
                b16Array.resize(imageWidth * imageHeight * 2);
                const quint8* src = reinterpret_cast<const quint8*>(imageData.constData());
                quint8* dst = reinterpret_cast<quint8*>(b16Array.data());
                for (int i = 0; i < imageWidth * imageHeight / 2; ++i)
                {
                    unpack12BitLittleEndianAndShift(src, dst, &dst[2]);
                    // - two 16 bit ints = 4 bytes
                    std::advance(dst, 4);
                    // - two 12 bit ints = 3 bytes when packed
                    std::advance(src, 3);
                }
            }
            break;
            case PixelFormat::BayerGR16:
            case PixelFormat::BayerBG16:
            case PixelFormat::BayerGB16:
            case PixelFormat::BayerRG16:
            case PixelFormat::Mono16:
                b16Array = imageData;
                break;
            default:
                qCritical() << "Unknown PixelFormat in MacsImage::data16unpacked - returning empty ByteArray";
            }
        }
        else
        {
            qCritical() << "Unknown PixelEndianness in MacsImage::data16unpacked - returning empty ByteArray";
        }
        return b16Array;
    }

    /* layout of MacsMetaData serialisation
     * ------------------------------------
     * [KEY][DATA][KEY][DATA]....'EOR#'
     *  KEY is a 32bit integer, where
     *      upper 16bit denotes an identifier
     *      lower 16bit denotes the data type
     *  DATA is prefixed with its size (int32), if it holds variable length data (e.g. string)
     *  (multi byte data is stored in big endian!)
     */

    namespace
    {
        const qint32 EOR = ('E' << 24) + ('O' << 16) + ('R' << 8) + '#';

        const qint16 INT32_T = 0x0004;
        const qint16 STRING_T = 0x0100;

        const qint32 CAM_VENDOR = (0x01 << 16) + STRING_T;
        const qint32 CAM_MODEL = (0x02 << 16) + STRING_T;
        const qint32 CAM_NAME = (0x03 << 16) + STRING_T;
        const qint32 CAM_SERIAL = (0x04 << 16) + STRING_T;
        const qint32 CAM_MAC = (0x05 << 16) + STRING_T;
        const qint32 CAM_IP = (0x06 << 16) + STRING_T;
        const qint32 CAM_FW = (0x07 << 16) + STRING_T;
        const qint32 IMG_ID = (0x11 << 16) + INT32_T;
        const qint32 IMG_IDX = (0x12 << 16) + INT32_T;
        const qint32 IMG_TAP = (0x13 << 16) + INT32_T;
        const qint32 IMG_EXP = (0x14 << 16) + INT32_T;
        const qint32 IMG_TIME = (0x15 << 16) + INT32_T;
        const qint32 AFFIX = (0x1F << 16) + STRING_T;
        const qint32 COMMENT = (0x21 << 16) + STRING_T;
    }; // namespace

    QDataStream& operator<<(QDataStream& out, const MacsMetaData& meta)
    {
        if (!meta.camVendor.isNull())
            out << CAM_VENDOR << meta.camVendor.left(32);
        if (!meta.camModel.isNull())
            out << CAM_MODEL << meta.camModel.left(32);
        if (!meta.camName.isNull())
            out << CAM_NAME << meta.camName.left(32);
        if (!meta.camSerial.isNull())
            out << CAM_SERIAL << meta.camSerial.left(32);
        if (!meta.camMAC.isNull())
            out << CAM_MAC << meta.camMAC.left(32);
        if (!meta.camIP.isNull())
            out << CAM_IP << meta.camIP.left(32);
        if (!meta.camFirmware.isNull())
            out << CAM_FW << meta.camFirmware.left(32);
        if (meta.imageID >= 0)
            out << IMG_ID << meta.imageID;
        if (meta.imageIDX >= 0)
            out << IMG_IDX << meta.imageIDX;
        if (meta.tapCount >= 0)
            out << IMG_TAP << meta.tapCount;
        if (meta.expTimeUS >= 0)
            out << IMG_EXP << meta.expTimeUS;
        if (meta.timeStamp >= 0)
            out << IMG_TIME << meta.timeStamp;
        if (!meta.comment.isNull())
            out << COMMENT << meta.comment.left(128);
        if (!meta.affix.isNull())
            out << AFFIX << meta.affix.left(32);
        out << EOR;
        return out;
    }

    QDataStream& operator>>(QDataStream& in, MacsMetaData& meta)
    {
        qint32 key;
        while (!in.atEnd())
        {
            in >> key;
            switch (key)
            {
            case EOR:
                return in;
            case CAM_VENDOR:
                in >> meta.camVendor;
                break;
            case CAM_MODEL:
                in >> meta.camModel;
                break;
            case CAM_NAME:
                in >> meta.camName;
                break;
            case CAM_SERIAL:
                in >> meta.camSerial;
                break;
            case CAM_MAC:
                in >> meta.camMAC;
                break;
            case CAM_IP:
                in >> meta.camIP;
                break;
            case CAM_FW:
                in >> meta.camFirmware;
                break;
            case IMG_ID:
                in >> meta.imageID;
                break;
            case IMG_IDX:
                in >> meta.imageIDX;
                break;
            case IMG_TAP:
                in >> meta.tapCount;
                break;
            case IMG_EXP:
                in >> meta.expTimeUS;
                break;
            case IMG_TIME:
                in >> meta.timeStamp;
                break;
            case COMMENT:
                in >> meta.comment;
                break;
            case AFFIX:
                in >> meta.affix;
                break;
            default: {
                qint16 type = key & 0xFFFF;
                if (type == INT32_T)
                {
                    qint32 dummy;
                    in >> dummy;
                    log(Log::Warning) << QString("Read unexpected %1 record in MacsMetaData. key: %2 value: %3").arg("int32").arg(key >> 16).arg(dummy);
                }
                else if (type == STRING_T)
                {
                    QByteArray dummy;
                    in >> dummy;
                    log(Log::Warning) << QString("Read unexpected %1 record in MacsMetaData. key: %2 value: %3")
                                             .arg("string")
                                             .arg(key >> 16)
                                             .arg(QString::fromUtf8(dummy));
                }
                else
                {
                    qint32 length = type & 0xFF;
                    if (length == 0)
                        in >> length;
                    QByteArray dummy(length, 0);
                    in.readRawData(dummy.data(), length);
                    log(Log::Warning) << QString("Read unexpected record in MacsMetaData. key: %1 type: %2 value: %3")
                                             .arg(key >> 16)
                                             .arg(type)
                                             .arg(QString::fromUtf8(dummy));
                }
                break;
            }
            }
        }
        return in;
    }

    MacsMetaData::MacsMetaData(const MacsCameraInfo& mci, const MacsPhotoInfo& mpi, const QByteArray& note)
        : camVendor(mci.vendor)
        , camModel(mci.model)
        , camName(mci.name)
        , camSerial(mci.serial)
        , camMAC(mci.mac)
        , camIP(mci.ip)
        , camFirmware(mci.firmware)
        , imageID(mpi.imageID)
        , imageIDX(mpi.imageIDX)
        , tapCount(mpi.tapCount)
        , expTimeUS(mpi.expTimeUS)
        , timeStamp(mpi.timeStamp)
        , affix(mpi.affix)
        , comment(note)
    {}

    MacsMetaData::MacsMetaData(const QByteArray& data)
    {
        QDataStream stream(data);
        stream.setVersion(QDataStream::Qt_5_4);
        stream >> *this;
    }

} // namespace MACS


