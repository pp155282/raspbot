#include "stream.h"

VideoStream::VideoStream(SourceType sourceType, std::string fname) :
 m_fname(fname),
 m_sourceType(sourceType),
 m_h264source(fname)
{
    switch (g_source)
    {
        case SOCKETIN:
        case STDIN:
        {
            m_reader = new cv::gpu::VideoReader_GPU(cv::Ptr<H264StreamSource>(&m_h264source));
            break;
        }
        case FILEIN:
        {
            m_reader = new cv::gpu::VideoReader_GPU(fname);
            break;
        }
        case WEBCAM:
        {
            webcam.open(0);
            webcam.set(cv::CAP_PROP_FRAME_WIDTH, 800);
            webcam.set(cv::CAP_PROP_FRAME_HEIGHT, 600);
            if  (!webcam.isOpened()) 
            {
                std::cout << "cannot open camera";
            }
            break;
        }
    }

    if (m_reader) {
        m_reader->dumpFormat(std::cout);
    }
}

bool VideoStream::readLatestFrame()
{
     switch (g_source)
    {
        case SOCKETIN:
        case STDIN:
        {
            //tm.reset(); tm.start();
            // drain queue by reading lots of frames. Note OpenCV was altered to allow this to
            // happen. normally Open cv does:
            // // Wait a bit
            // detail::Thread::sleep(1); 
            while (m_reader->read(m_gCurrentFrame) || !m_gCurrentFrame.rows)
            {
                // no frame at all, wait a bit for one to arrive
                if (!m_gCurrentFrame.rows)
                    Sleep(1);
                //continue;
            }
            //tm.stop();
            //gpu_times.push_back(tm.getTimeMilli());
            break;
        }
        case FILEIN:
            {
                //tm.reset(); tm.start();
                if (!m_reader->read(m_gCurrentFrame))
                {
                    m_reader->close();
                    m_reader->open(m_fname);
                    m_reader->read(m_gCurrentFrame);
                }
                //tm.stop();
                //gpu_times.push_back(tm.getTimeMilli());
                break;
            }
        case WEBCAM:
            {
            webcam.read(m_cCurrentFrame);
            m_gCurrentFrame.upload(m_cCurrentFrame);
            break;
        }
    }

    //cv::gpu::resize(d_frame, d_frame2,cv::Size(512,512));
    cv::gpu::cvtColor(m_gCurrentFrame, m_gCurrentFrameGray, (g_source == WEBCAM ? CV_RGB2GRAY : CV_RGBA2GRAY));

    if (g_useFilter)
    {
        cv::gpu::GpuMat tmp;
        cv::gpu::bilateralFilter(m_gCurrentFrameGray,tmp,30,24,6.5);
        // just throw away tmp? does the copy happen or more just a "move"?
        m_gCurrentFrameGray = tmp;
    }

    return true;
}



