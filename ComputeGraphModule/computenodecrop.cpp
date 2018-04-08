#include "computenodecrop.h"

#include <opencv2/opencv.hpp>

ComputeNodeCrop::ComputeNodeCrop(const QString& name)
    : GtComputeNodeBase(name)
    , top(name+"/top", 0, 0, 600)
    , left(name+"/left", 0, 0, 600)
    , width(name+"/width", 300, 0, 600)
    , height(name+"/height", 300, 0, 600)
{
}

bool ComputeNodeCrop::onInputChanged(const cv::Mat* input)
{
    output->create(height, width, input->type());

    return true;
}

void ComputeNodeCrop::update(const cv::Mat* input)
{
    validateRect(top, left, width, height, input);
    cv::Rect rect(left, top, width, height);
    cv::Mat roi = (*input)(rect);
    *output = roi.clone();
}

void ComputeNodeCrop::validateRect(qint32& top, qint32& left, qint32& width, qint32& height, const cv::Mat* input)
{
    qint32 max_width = input->cols;
    qint32 max_height = input->rows;
    left = qBound(0, left, max_width - 1);
    top = qBound(0, top, max_height - 1);
    width = qBound(1, width, max_width - left);
    height = qBound(1, height, max_height - top);
    if(output->rows != height || output->cols != width) {
        output->create(height, width, input->type());
        outputChanged();
    }
}
