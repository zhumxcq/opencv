/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "precomp.hpp"
#include "cap_dshow.hpp"

#include "opencv2/videoio/registry.hpp"
#include "videoio_registry.hpp"

namespace cv {

template<> void DefaultDeleter<CvCapture>::operator ()(CvCapture* obj) const
{ cvReleaseCapture(&obj); }

template<> void DefaultDeleter<CvVideoWriter>::operator ()(CvVideoWriter* obj) const
{ cvReleaseVideoWriter(&obj); }

/*!!!!!!!-------------------ADDED BY E-CON SYSTEMS----------!!!!!!!! */
static inline bool icvGetVideoProperty(CvCapture* capture, int id, int& min, int& max, int& steppingDelta, int& supportedMode, int& currentValue, int& currentMode, int& defaultValue)
{
    return capture ? capture->getProperty(id, min, max, steppingDelta, supportedMode, currentValue, currentMode, defaultValue) : 0;
}
/*!!!!!!!---------------------------END-----------------------!!!!!!!! */
/*!!!!!!!-------------------ADDED BY E-CON SYSTEMS----------!!!!!!!! */
CV_IMPL bool cvGetFormats(CvCapture* capture, int& formats)
{
    return capture ? capture->getFormats(formats) : 0;
}

CV_IMPL bool cvGetFormatType(CvCapture* capture, int formats, String& formatType, int& width, int& height, int& fps)
{
    return capture ? capture->getFormatType(formats, formatType, width, height, fps) : 0;
}

CV_IMPL bool cvSetFormatType(CvCapture* capture, int index)
{
    return capture ? capture->setFormatType(index) : 0;
}
/*!!!!!!!---------------------------END-----------------------!!!!!!!! */
    /*!!!!!!!-------------------ADDED BY E-CON SYSTEMS----------!!!!!!!! */
CV_IMPL bool cvSetVideoProperty(CvCapture* capture, int id, int value, int mode)
{
    return capture ? capture->setProperty(id, value, mode) : 0;
}
/*!!!!!!!---------------------------END-----------------------!!!!!!!! */
/*!!!!!!!-------------------ADDED BY E-CON SYSTEMS----------!!!!!!!! */
CV_IMPL CvCapture* cvGetDevices(int& devices)
{
    int pref = 0;
    CvCapture* capture = 0;

    switch (pref)
    {
    default:
        if (pref)
            break;

    case CAP_VFW: // or CAP_V4L or CAP_V4L2

#if defined HAVE_LIBV4L || defined HAVE_CAMV4L || defined HAVE_CAMV4L2 || defined HAVE_VIDEOIO
        TRY_OPEN(capture, cvGetDevices_V4L(devices))
#endif
            if (pref) break; // CAP_VFW or CAP_V4L or CAP_V4L2
    }
    return capture;
}

CV_IMPL CvCapture* cvGetDeviceInfo(int index, String& deviceName, String& vid, String& pid, String& devicePath)
{

    int pref = 0;
    CvCapture* capture = 0;
    switch (pref)
    {
    default:
        if (pref)    break;

    case CAP_VFW: // or CAP_V4L or CAP_V4L2
#if defined HAVE_LIBV4L || defined HAVE_CAMV4L || defined HAVE_CAMV4L2 || defined HAVE_VIDEOIO
        TRY_OPEN(capture, cvGetDeviceInfo_V4L(index, deviceName, vid, pid, devicePath))
#endif
            if (pref)    break; //CAP_VFW  or CAP_V4L or CAP_V4L2
    }
    return capture;
    //return capture ? capture->getDeviceInfo(index, deviceName, vid, pid, devicePath) : 0;
}
/*!!!!!!!---------------------------END-----------------------!!!!!!!! */

VideoCapture::VideoCapture()
{}

VideoCapture::VideoCapture(const String& filename, int apiPreference)
{
    CV_TRACE_FUNCTION();
    open(filename, apiPreference);
}

VideoCapture::VideoCapture(const String& filename)
{
    CV_TRACE_FUNCTION();
    open(filename, CAP_ANY);
}

VideoCapture::VideoCapture(int index)
{
    CV_TRACE_FUNCTION();
    open(index);
}

VideoCapture::VideoCapture(int index, int apiPreference)
{
    CV_TRACE_FUNCTION();
    open(index, apiPreference);
}

VideoCapture::~VideoCapture()
{
    CV_TRACE_FUNCTION();

    icap.release();
    cap.release();
}

bool VideoCapture::open(const String& filename, int apiPreference)
{
    CV_TRACE_FUNCTION();

    if (isOpened()) release();

    const std::vector<VideoBackendInfo> backends = cv::videoio_registry::getAvailableBackends_CaptureByFilename();
    for (size_t i = 0; i < backends.size(); i++)
    {
        const VideoBackendInfo& info = backends[i];
        if (apiPreference == CAP_ANY || apiPreference == info.id)
        {
            CvCapture* capture = NULL;
            VideoCapture_create(capture, icap, info.id, filename);
            if (!icap.empty())
            {
                if (icap->isOpened())
                    return true;
                icap.release();
            }
            if (capture)
            {
                cap.reset(capture);
                // assume it is opened
                return true;
            }
        }
    }
    return false;
}

bool VideoCapture::open(const String& filename)
{
    CV_TRACE_FUNCTION();

    return open(filename, CAP_ANY);
}

bool  VideoCapture::open(int cameraNum, int apiPreference)
{
    CV_TRACE_FUNCTION();

    if (isOpened()) release();

    if (apiPreference == CAP_ANY)
    {
        // interpret preferred interface (0 = autodetect)
        int backendID = (cameraNum / 100) * 100;
        if (backendID)
        {
            cameraNum %= 100;
            apiPreference = backendID;
        }
    }

    const std::vector<VideoBackendInfo> backends = cv::videoio_registry::getAvailableBackends_CaptureByIndex();
    for (size_t i = 0; i < backends.size(); i++)
    {
        const VideoBackendInfo& info = backends[i];
        if (apiPreference == CAP_ANY || apiPreference == info.id)
        {
            CvCapture* capture = NULL;
            VideoCapture_create(capture, icap, info.id, cameraNum);
            if (!icap.empty())
            {
                if (icap->isOpened())
                    return true;
                icap.release();
            }
            if (capture)
            {
                cap.reset(capture);
                // assume it is opened
                return true;
            }
        }
    }
    return false;
}

bool VideoCapture::open(int index)
{
    CV_TRACE_FUNCTION();

    return open(index, CAP_ANY);
}

bool VideoCapture::isOpened() const
{
    if (!icap.empty())
        return icap->isOpened();
    return !cap.empty();  // legacy interface doesn't support closed files
}

CV_WRAP bool VideoCapture::isConnected() const
{
    if (!icap.empty())
        return icap->isConnected();
    return !cap.empty();
}

String VideoCapture::getBackendName() const
{
    int api = 0;
    if (icap)
        api = icap->isOpened() ? icap->getCaptureDomain() : 0;
    else if (cap)
        api = cap->getCaptureDomain();
    CV_Assert(api != 0);
    return cv::videoio_registry::getBackendName((VideoCaptureAPIs)api);
}

void VideoCapture::release()
{
    CV_TRACE_FUNCTION();
    icap.release();
    cap.release();
}

/*!!!!!!!-------------------ADDED BY E-CON SYSTEMS----------!!!!!!!! */
bool VideoCapture::getDevices(CV_OUT int& devices)
{
    CV_TRACE_FUNCTION();
#ifdef HAVE_DSHOW
    VideoCapture_DShow GD;
    return GD.getDevices(devices);
#endif
#if defined HAVE_LIBV4L || defined HAVE_CAMV4L || defined HAVE_CAMV4L2 || defined HAVE_VIDEOIO
    cap.reset(cvGetDevices(devices));
    return true;
#endif
    return false;
}

bool VideoCapture::getDeviceInfo(int index, CV_OUT String& deviceName, CV_OUT String& vid, CV_OUT String& pid, CV_OUT String& devicePath)
{
    CV_TRACE_FUNCTION();
#ifdef HAVE_DSHOW
    VideoCapture_DShow GDI;
    return GDI.getDeviceInfo(index, deviceName, vid, pid, devicePath);
#endif
#if defined HAVE_LIBV4L || defined HAVE_CAMV4L || defined HAVE_CAMV4L2 || defined HAVE_VIDEOIO
    cap.reset(cvGetDeviceInfo(index, deviceName, vid, pid, devicePath));
    return true;
#endif
    return false;
}

bool VideoCapture::getFormats(CV_OUT int& formats)
{
    CV_TRACE_FUNCTION();
    if (!icap.empty())
        return icap->getFormats(formats);
    return cvGetFormats(cap, formats);
}

bool VideoCapture::getFormatType(int formats, CV_OUT String& formatType, CV_OUT int& width, CV_OUT int& height, CV_OUT int& fps)
{
    CV_TRACE_FUNCTION();
    if (!icap.empty())
        return icap->getFormatType(formats, formatType, width, height, fps);
    return cvGetFormatType(cap, formats, formatType, width, height, fps);
}

bool VideoCapture::setFormatType(int index)
{
    CV_TRACE_FUNCTION();
    String formatType;
    int width, height, fps;
    if (!icap.empty())
    {
        if (icap->getFormatType(index, formatType, width, height, fps))
        {
            if (icap->setProperty(CV_CAP_PROP_FOURCC, index))
            {
                if (icap->setProperty(CV_CAP_PROP_FRAME_WIDTH, width))
                {
                    if (icap->setProperty(CV_CAP_PROP_FRAME_HEIGHT, height))
                    {
                        return icap->setProperty(CV_CAP_PROP_FPS, fps);
                    }
                }
            }
        }

    }

    return cvSetFormatType(cap, index);
}
/*!!!!!!!---------------------------END-----------------------!!!!!!!! */

bool VideoCapture::grab()
{
    CV_INSTRUMENT_REGION();

    if (!icap.empty())
        return icap->grabFrame();
    return cvGrabFrame(cap) != 0;
}

bool VideoCapture::retrieve(OutputArray image, int channel)
{
    CV_INSTRUMENT_REGION();

    if (!icap.empty())
        return icap->retrieveFrame(channel, image);

    IplImage* _img = cvRetrieveFrame(cap, channel);
    if( !_img )
    {
        image.release();
        return false;
    }
    if(_img->origin == IPL_ORIGIN_TL)
        cv::cvarrToMat(_img).copyTo(image);
    else
    {
        Mat temp = cv::cvarrToMat(_img);
        flip(temp, image, 0);
    }
    return true;
}

bool VideoCapture::read(OutputArray image)
{
    CV_INSTRUMENT_REGION();

    if(grab())
        retrieve(image);
    else
        image.release();
    return !image.empty();
}

VideoCapture& VideoCapture::operator >> (Mat& image)
{
#ifdef WINRT_VIDEO
    // FIXIT grab/retrieve methods() should work too
    if (grab())
    {
        if (retrieve(image))
        {
            std::lock_guard<std::mutex> lock(VideoioBridge::getInstance().inputBufferMutex);
            VideoioBridge& bridge = VideoioBridge::getInstance();

            // double buffering
            bridge.swapInputBuffers();
            auto p = bridge.frontInputPtr;

            bridge.bIsFrameNew = false;

            // needed here because setting Mat 'image' is not allowed by OutputArray in read()
            Mat m(bridge.getHeight(), bridge.getWidth(), CV_8UC3, p);
            image = m;
        }
    }
#else
    read(image);
#endif
    return *this;
}

VideoCapture& VideoCapture::operator >> (UMat& image)
{
    CV_INSTRUMENT_REGION();

    read(image);
    return *this;
}

bool VideoCapture::set(int propId, double value)
{
    CV_CheckNE(propId, (int)CAP_PROP_BACKEND, "Can set read-only property");

    if (!icap.empty())
        return icap->setProperty(propId, value);
    return cvSetCaptureProperty(cap, propId, value) != 0;
}

/*!!!!!!!-------------------ADDED BY E-CON SYSTEMS----------!!!!!!!! */
bool VideoCapture::set(int propId, int value, int mode)
{
    //    CV_TRACE_FUNCTION();
    if (!icap.empty())
        return icap->setVideoProperty(propId, value, mode);
    return cvSetVideoProperty(cap, propId, value, mode) != 0;
}

double VideoCapture::get(int propId) const
{
    if (propId == CAP_PROP_BACKEND)
    {
        int api = 0;
        if (icap)
            api = icap->isOpened() ? icap->getCaptureDomain() : 0;
        else if (cap)
            api = cap->getCaptureDomain();
        if (api <= 0)
            return -1.0;
        return (double)api;
    }
    if (!icap.empty())
        return icap->getProperty(propId);
    return cap ? cap->getProperty(propId) : 0;
}

/*!!!!!!!-------------------ADDED BY E-CON SYSTEMS----------!!!!!!!! */
bool VideoCapture::get(int propId, CV_IN_OUT int& min, CV_IN_OUT int& max, CV_IN_OUT int& steppingDelta, CV_IN_OUT int& supportedMode, CV_IN_OUT int& currentValue, CV_IN_OUT int& currentMode, CV_IN_OUT int& defaultValue)
{
    //    CV_TRACE_FUNCTION();
    if (!icap.empty())
        return icap->getVideoProperty(propId, min, max, steppingDelta, supportedMode, currentValue, currentMode, defaultValue);
    return icvGetVideoProperty(cap, propId, min, max, steppingDelta, supportedMode, currentValue, currentMode, defaultValue);
}
/*!!!!!!!---------------------------END-----------------------!!!!!!!! */

//=================================================================================================



VideoWriter::VideoWriter()
{}

VideoWriter::VideoWriter(const String& filename, int _fourcc, double fps, Size frameSize, bool isColor)
{
    open(filename, _fourcc, fps, frameSize, isColor);
}


VideoWriter::VideoWriter(const String& filename, int apiPreference, int _fourcc, double fps, Size frameSize, bool isColor)
{
    open(filename, apiPreference, _fourcc, fps, frameSize, isColor);
}

void VideoWriter::release()
{
    iwriter.release();
    writer.release();
}

VideoWriter::~VideoWriter()
{
    release();
}

bool VideoWriter::open(const String& filename, int _fourcc, double fps, Size frameSize, bool isColor)
{
    return open(filename, CAP_ANY, _fourcc, fps, frameSize, isColor);
}

bool VideoWriter::open(const String& filename, int apiPreference, int _fourcc, double fps, Size frameSize, bool isColor)
{
    CV_INSTRUMENT_REGION();

    if (isOpened()) release();

    const std::vector<VideoBackendInfo> backends = cv::videoio_registry::getAvailableBackends_Writer();
    for (size_t i = 0; i < backends.size(); i++)
    {
        const VideoBackendInfo& info = backends[i];
        if (apiPreference == CAP_ANY || apiPreference == info.id)
        {
            CvVideoWriter* writer_ = NULL;
            VideoWriter_create(writer_, iwriter, info.id, filename, _fourcc, fps, frameSize, isColor);
            if (!iwriter.empty())
            {
                if (iwriter->isOpened())
                    return true;
                iwriter.release();
            }
            if (writer_)
            {
                // assume it is opened
                writer.reset(writer_);
                return true;
            }
        }
    }
    return false;
}

bool VideoWriter::isOpened() const
{
    return !iwriter.empty() || !writer.empty();
}


bool VideoWriter::set(int propId, double value)
{
    CV_CheckNE(propId, (int)CAP_PROP_BACKEND, "Can set read-only property");

    if (!iwriter.empty())
        return iwriter->setProperty(propId, value);
    return false;
}

double VideoWriter::get(int propId) const
{
    if (propId == CAP_PROP_BACKEND)
    {
        int api = 0;
        if (iwriter)
            api = iwriter->getCaptureDomain();
        else if (writer)
            api = writer->getCaptureDomain();
        if (api <= 0)
            return -1.0;
        return (double)api;
    }
    if (!iwriter.empty())
        return iwriter->getProperty(propId);
    return 0.;
}

String VideoWriter::getBackendName() const
{
    int api = 0;
    if (iwriter)
        api = iwriter->getCaptureDomain();
    else if (writer)
        api = writer->getCaptureDomain();
    CV_Assert(api != 0);
    return cv::videoio_registry::getBackendName((VideoCaptureAPIs)api);
}

void VideoWriter::write(const Mat& image)
{
    CV_INSTRUMENT_REGION();

    if( iwriter )
        iwriter->write(image);
    else
    {
        IplImage _img = cvIplImage(image);
        cvWriteFrame(writer, &_img);
    }
}

VideoWriter& VideoWriter::operator << (const Mat& image)
{
    CV_INSTRUMENT_REGION();

    write(image);
    return *this;
}

// FIXIT OpenCV 4.0: make inline
int VideoWriter::fourcc(char c1, char c2, char c3, char c4)
{
    return (c1 & 255) + ((c2 & 255) << 8) + ((c3 & 255) << 16) + ((c4 & 255) << 24);
}

} // namespace
