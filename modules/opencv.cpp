#include "opencv.h"
#include <QDebug>
#include <QFileDialog>
#include <QQueue>
#include <QtMath>
#include <QRandomGenerator>
#include <set>

using namespace cv;

FuncPanel::FuncPanel(const QString &title, const QStringList &items, QWidget *parent):
    QWidget(parent)
{
    this->title = new QLabel(title);
    listWidget = new QListWidget();

    this->title->setAlignment(Qt::AlignCenter);
    for (const auto &item : items) {
        QListWidgetItem *witem = new QListWidgetItem(item);
        witem->setTextAlignment(Qt::AlignCenter);
        listWidget->addItem(witem);
    }
    listWidget->setWordWrap(true);
    listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    layout = new QVBoxLayout();
    layout->addWidget(this->title);
    layout->addWidget(listWidget, 1);
    layout->setContentsMargins(0, 0, 0, 0);
    // 不让标签把列尺寸撑大
    this->title->setSizePolicy(QSizePolicy::Ignored,
                               QSizePolicy::Preferred);
    setLayout(layout);
}

FuncPanel::~FuncPanel() {}

bool Opencv::noisePresent = false;

Opencv::Opencv(QWidget *parent):
    QWidget(parent)
{
    // 初始化布局
    layoutInit();

    // 显示默认图像
    QImage qSource(DEFAULT_SOURCE_IMAGE);
    qSource = qSource.convertToFormat(QImage::Format_RGB888);
    qSource = qSource.scaled(sourceImageLabel->size(),
        Qt::KeepAspectRatio, Qt::FastTransformation);
    sourceImage = Mat(
        qSource.height(),
        qSource.width(),
        CV_8UC3,
        qSource.bits(),
        qSource.bytesPerLine()
        ).clone();

    cvtColor(sourceImage, grayImage, COLOR_RGB2GRAY);
    QImage qGray = matToQImage(grayImage);

    sourceImageLabel->setPixmap(QPixmap::fromImage(qSource));
    imageSlotLabels[0]->setPixmap(QPixmap::fromImage(qGray));

    // 初始化信号与槽
    connect(selectButton, &QPushButton::clicked,
            this, &Opencv::selectButtonClicked);
    connect(procPanel->listWidget, &QListWidget::itemClicked,
            this, &Opencv::procPanelClicked);
    connect(filterPanel->listWidget, &QListWidget::itemClicked,
            this, &Opencv::filterPanelClicked);
    connect(distinctPanel->listWidget, &QListWidget::itemClicked,
            this, &Opencv::distinctPanelClicked);
    connect(noisePanel->listWidget, &QListWidget::itemClicked,
            this, &Opencv::noisePanelClicked);
    connect(cameraPanel->listWidget, &QListWidget::itemClicked,
            this, &Opencv::cameraPanelClicked);
    connect(transformPanel->listWidget, &QListWidget::itemClicked,
            this, &Opencv::transformPanelClicked);
    connect(edgePanel->listWidget, &QListWidget::itemClicked,
            this, &Opencv::edgePanelClicked);
    connect(SegmentationPanel->listWidget, &QListWidget::itemClicked,
            this, &Opencv::SegmentationPanelClicked);
    connect(characterPanel->listWidget, &QListWidget::itemClicked,
            this, &Opencv::characterPanelClicked);
}

Opencv::~Opencv() {
    slotImagesClear();
    sourceImage.release();
}

void Opencv::layoutInit() {
    sourceImageLabel = new QLabel();
    imageSlotLabels[0] = new QLabel();
    imageSlotLabels[1] = new QLabel();
    imageSlotLabels[2] = new QLabel();
    selectButton = new QPushButton("Select Image");

    sourceImageLabel->setAlignment(Qt::AlignCenter);
    imageSlotLabels[0]->setAlignment(Qt::AlignCenter);
    imageSlotLabels[1]->setAlignment(Qt::AlignCenter);
    imageSlotLabels[2]->setAlignment(Qt::AlignCenter);

    QStringList items;
    items << "Process"
          << "Histogram"
          << "Equalization"
          << "Gradient"
          << "Laplace";
    procPanel = new FuncPanel("Process", items);

    items.clear();
    items << "Mean"
          << "Median"
          << "Window"
          << "Morphological"
          << "Gaussian";
    filterPanel = new FuncPanel("Filter", items);

    items.clear();
    items << "LBP"
          << "Histogram Detection"
          << "Template Matching"
          << "Color Matching"
          << "Gabor Filtering";
    distinctPanel = new FuncPanel("Distinct", items);

    items.clear();
    items << "Salt&Pepper"
          << "Gaussian";
    noisePanel = new FuncPanel("Noise", items);

    items.clear();
    items << "Camera Calibration"
          << "Stereo Matching";
    cameraPanel = new FuncPanel("Camera", items);


    items.clear();
    items << "Affine"
          << "Perspective";
    transformPanel = new FuncPanel("Transform", items);

    items.clear();
    items << "Roberts"
          << "Sobel"
          << "Laplace"
          << "Prewitt"
          << "Canny";
    edgePanel = new FuncPanel("Edge", items);

    items.clear();
    items << "Threshold"
          << "OSTU"
          << "Kittler"
          << "Inter-frame Difference"
          << "Gaussian Mixture";
    SegmentationPanel = new FuncPanel("Segmentation", items);

    items.clear();
    items << "ORB"
          << "SVM"
          << "Test"
          << "Haar-V"
          << "Haar-H";
    characterPanel = new FuncPanel("Character", items);

    // 显示界面布局
    gDisplayLayout = new QGridLayout();
    int imgSize = 135;
    sourceImageLabel->setFixedSize(imgSize, imgSize);
    imageSlotLabels[0]->setFixedSize(imgSize, imgSize);
    imageSlotLabels[1]->setFixedSize(imgSize, imgSize);
    imageSlotLabels[2]->setFixedSize(imgSize, imgSize);
    gDisplayLayout->addWidget(sourceImageLabel, 0, 0);
    gDisplayLayout->addWidget(imageSlotLabels[0], 0, 1);
    gDisplayLayout->addWidget(selectButton, 1, 0, 1, 2);
    gDisplayLayout->addWidget(imageSlotLabels[1], 2, 0);
    gDisplayLayout->addWidget(imageSlotLabels[2], 2, 1);

    // 控制面板布局
    gControlLayout = new QGridLayout();

    gControlLayout->addWidget(procPanel, 0, 0);
    gControlLayout->addWidget(filterPanel, 0, 1);
    gControlLayout->addWidget(distinctPanel, 0, 2);
    gControlLayout->addWidget(noisePanel, 1, 0);
    gControlLayout->addWidget(cameraPanel, 1, 1);
    gControlLayout->addWidget(transformPanel, 1, 2);
    gControlLayout->addWidget(edgePanel, 2, 0);
    gControlLayout->addWidget(SegmentationPanel, 2, 1);
    gControlLayout->addWidget(characterPanel, 2, 2);

    gControlLayout->setRowStretch(0, 5);
    gControlLayout->setRowStretch(1, 2);
    gControlLayout->setRowStretch(2, 5);
    gControlLayout->setColumnStretch(0, 1);
    gControlLayout->setColumnStretch(1, 1);
    gControlLayout->setColumnStretch(2, 1);

    // 主界面
    hMainLayout = new QHBoxLayout();
    hMainLayout->addLayout(gDisplayLayout);
    hMainLayout->addLayout(gControlLayout);
    setLayout(hMainLayout);
}

QImage Opencv::matToQImage(const Mat &mat) {
    QImage::Format fmt;
    if (mat.channels() == 1)
        fmt = QImage::Format_Grayscale8;
    else
        fmt = QImage::Format_RGB888;

    return QImage(mat.data,
                  mat.cols,
                  mat.rows,
                  mat.step,
                  fmt);
}

void Opencv::slotImagesClear() {
    for (QLabel *label : imageSlotLabels) {
        label->clear();
    }
    clearNoisedImage();
    clearGrayImage();
}

void Opencv::selectButtonClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open File",
                                                    "/home/menoa/image",
                                                    "Images (*.png *.jpg)");
    QImage qSource(fileName);
    qSource = qSource.convertToFormat(QImage::Format_RGB888);
    qSource = qSource.scaled(sourceImageLabel->size(),
                             Qt::KeepAspectRatio, Qt::FastTransformation);
    sourceImage = Mat(qSource.height(),
                      qSource.width(),
                      CV_8UC3,
                      qSource.bits(),
                      qSource.bytesPerLine()
                      ).clone();

    cvtColor(sourceImage, grayImage, COLOR_RGB2GRAY);
    sourceImageLabel->setPixmap(QPixmap::fromImage(qSource));
    slotImagesClear();
}

void Opencv::procPanelClicked(QListWidgetItem *item) {
    slotImagesClear();
    if (item->text() == "Process")
        grayProcess();
    else if (item->text() == "Histogram")
        grayHistogram();
    else if (item->text() == "Equalization")
        grayBalance();
    else if (item->text() == "Gradient")
        return;
    else if (item->text() == "Laplace")
        laplaceSharpen();

    clearNoisedImage();
}

void Opencv::filterPanelClicked(QListWidgetItem *item) {
    if (!noisePresent)
        return;
    if (item->text() == "Mean")
        averageFilter();
    else if (item->text() == "Median")
        middleFilter();
    else if (item->text() == "Window")
        windowFilter();
    else if (item->text() == "Morphological")
        morphFilter();
    else if (item->text() == "Gaussian")
        gaussFilter();
}

void Opencv::distinctPanelClicked(QListWidgetItem *item) {
    slotImagesClear();
    if (item->text() == "LBP")
        return;
    else if (item->text() == "Histogram Detection")
        return;
    else if (item->text() == "Template Matching")
        return;
    else if (item->text() == "Color Matching")
        return;
    else if (item->text() == "Gabor Filtering")
        return;
}

void Opencv::noisePanelClicked(QListWidgetItem *item) {
    slotImagesClear();
    if (item->text() == "Salt&Pepper")
        saltNoise();
    else if (item->text() == "Gaussian")
        gaussianNoise();

    clearGrayImage();
    noisePresent = true;
}

void Opencv::cameraPanelClicked(QListWidgetItem *item) {
    slotImagesClear();
    if (item->text() == "Camera Calibration")
        return;
    else if (item->text() == "Stereo Matching")
        return;
}

void Opencv::transformPanelClicked(QListWidgetItem *item) {
    slotImagesClear();
    if (item->text() == "Affine")
        affineTransform();
    else if (item->text() == "Perspective")
        perspectiveTransform();
}

void Opencv::edgePanelClicked(QListWidgetItem *item) {
    slotImagesClear();
    if (item->text() == "Roberts")
        robertsEdge();
    else if (item->text() == "Sobel")
        sobelEdge();
    else if (item->text() == "Laplace")
        laplaceSharpen();
    else if (item->text() == "Prewitt")
        prewittEdge();
    else if (item->text() == "Canny")
        cannyEdge();
}

void Opencv::SegmentationPanelClicked(QListWidgetItem *item) {
    slotImagesClear();
    if (item->text() == "Threshold")
        thresholdSeg();
    else if (item->text() == "OSTU")
        otsuSeg();
    else if (item->text() == "Kittler")
        kittlerSeg();
    else if (item->text() == "Inter-frame Difference")
        return;
    else if (item->text() == "Gaussian Mixture")
        return;
}

void Opencv::characterPanelClicked(QListWidgetItem *item) {
    slotImagesClear();
    if (item->text() == "ORB")
        return;
    else if (item->text() == "SVM")
        return;
    else if (item->text() == "Test")
        return;
    else if (item->text() == "Haar-V")
        return;
    else if (item->text() == "Haar-H")
        return;
}

void Opencv::clearGrayImage() {
    grayImage.release();
}

void Opencv::clearNoisedImage() {
    noisedImage.release();
    noisePresent = false;
}

// RGB转灰度图
void Opencv::grayProcess() {
    if (!grayImage.empty() || sourceImage.empty())
        return;

    // 释放后必须重新创建
    grayImage.create(sourceImage.rows, sourceImage.cols, CV_8UC1);

    for (int i = 0; i < sourceImage.rows; ++i) {
        for (int j = 0; j < sourceImage.cols; ++j) {
            grayImage.at<uchar>(i, j) =
                0.299 * sourceImage.at<Vec3b>(i, j)[0]
                + 0.587 * sourceImage.at<Vec3b>(i, j)[1]
                + 0.114 * sourceImage.at<Vec3b>(i, j)[2];
        }
    }

    QImage qGrayImage = matToQImage(grayImage);
    imageSlotLabels[0]->setPixmap(QPixmap::fromImage(qGrayImage));
}

// 灰度直方图
Mat Opencv::getGrayLevel(const Mat &gray) {
    QVector<int> pixel(256, 0);

    for (int i = 0; i < gray.rows; ++i) {
        for (int j = 0; j < gray.cols; ++j) {
            pixel[gray.at<uchar>(i, j)]++;
        }
    }

    int maxGray = 0;
    for (int i = 0; i < pixel.size(); ++i) {
        if (maxGray < pixel[i])
            maxGray = pixel[i];
    }

    // 先用前景铺满
    Mat grayLevel(256, 256, CV_8UC1);
    for (int i = 0; i < grayLevel.cols; ++i) {
        for (int j = 0; j < grayLevel.rows; ++j) {
            grayLevel.at<uchar>(j, i) = 0;
        }
    }

    // 再用背景覆盖
    for (int i = 0; i < grayLevel.cols; ++i) {
        int height = static_cast<int>(grayLevel.rows * 0.9 * pixel[i] / maxGray);
        for (int j = 0; j < grayLevel.rows - height; ++j) {
            grayLevel.at<uchar>(j, i) = 255;
        }
    }

    return grayLevel;
}

void Opencv::grayHistogram() {
    // 首先得到灰度图数据
    grayProcess();

    // 从灰度图得到直方图
    Mat grayLevel = getGrayLevel(grayImage);
    QImage qGrayLevel = matToQImage(grayLevel);
    qGrayLevel = qGrayLevel.scaled(imageSlotLabels[1]->size(),
                      Qt::KeepAspectRatio, Qt::FastTransformation);
    imageSlotLabels[1]->setPixmap(QPixmap::fromImage(qGrayLevel));
}

void Opencv::grayBalance() {
    Mat balance;
    balance.create(sourceImage.rows, sourceImage.cols, CV_8UC1);

    QVector<int> pixel(256, 0);
    QVector<float> grayPixel(256, 0.);
    float sum = 0;

    // 获取灰度图
    grayProcess();

    for (int i = 0; i < grayImage.rows; ++i) {
        for (int j = 0; j < grayImage.cols; ++j) {
            pixel[grayImage.at<uchar>(i, j)]++;
        }
    }

    for (const int val : pixel)
        sum += val;

    for (int i = 0; i < pixel.size(); ++i) {
        float num = 0;
        for (int j = 0; j <= i; ++j) {
            num += pixel[j];
        }
        grayPixel[i] = 255 * num / sum;
    }

    // 根据比例进行均衡
    for (int i = 0; i < sourceImage.rows; ++i) {
        for (int j = 0; j < sourceImage.cols; ++j) {
            balance.at<uchar>(i, j) =
                grayPixel[grayImage.at<uchar>(i, j)];
        }
    }

    QImage qBalance = matToQImage(balance);
    imageSlotLabels[1]->setPixmap(QPixmap::fromImage(qBalance));
}

void Opencv::laplaceSharpen() {
    Mat gradImg, grayImg;
    gradImg.create(sourceImage.cols, sourceImage.rows, CV_8UC1);
    grayImg.create(sourceImage.cols, sourceImage.rows, CV_8UC1);

    grayProcess();
    Mat &gray = grayImage;
    for (int i = 1; i < gradImg.rows - 1; ++i) {
        for (int j = 1; j < gradImg.cols - 1; ++j) {
            gradImg.at<uchar>(i, j) = saturate_cast<uchar>(
                -4 * gray.at<uchar>(i, j)
                + gray.at<uchar>(i - 1, j)
                + gray.at<uchar>(i, j - 1)
                + gray.at<uchar>(i, j + 1)
                + gray.at<uchar>(i + 1, j));

            grayImg.at<uchar>(i, j) = saturate_cast<uchar>(
                5 * gray.at<uchar>(i, j)
                - gray.at<uchar>(i - 1, j)
                - gray.at<uchar>(i, j - 1)
                - gray.at<uchar>(i, j + 1)
                - gray.at<uchar>(i + 1, j));
        }
    }

    QImage qGradImg = matToQImage(gradImg);
    QImage qGrayImg = matToQImage(grayImg);
    imageSlotLabels[1]->setPixmap(QPixmap::fromImage(qGradImg));
    imageSlotLabels[2]->setPixmap(QPixmap::fromImage(qGrayImg));
}

void Opencv::robertsEdge() {
    Mat gradImg, grayImg;
    gradImg.create(sourceImage.cols, sourceImage.rows, CV_8UC1);
    grayImg.create(sourceImage.cols, sourceImage.rows, CV_8UC1);

    grayProcess();
    Mat &gray = grayImage;
    for (int i = 1; i < gradImg.rows - 1; ++i) {
        for (int j = 1; j < gradImg.cols - 1; ++j) {
            gradImg.at<uchar>(i, j) = saturate_cast<uchar>(
                fabs(gray.at<uchar>(i, j) - gray.at<uchar>(i + 1, j + 1))
                + fabs(gray.at<uchar>(i + 1, j) - gray.at<uchar>(i, j + 1))
                );

            grayImg.at<uchar>(i, j) = saturate_cast<uchar>(
                gray.at<uchar>(i, j) - gradImg.at<uchar>(i, j)
                );
        }
    }

    QImage qGradImg = matToQImage(gradImg);
    QImage qGrayImg = matToQImage(grayImg);
    imageSlotLabels[1]->setPixmap(QPixmap::fromImage(qGradImg));
    imageSlotLabels[2]->setPixmap(QPixmap::fromImage(qGrayImg));
}

void Opencv::sobelEdge() {
    Mat gradImg, grayImg, fx, fy;
    gradImg.create(sourceImage.cols, sourceImage.rows, CV_8UC1);
    grayImg.create(sourceImage.cols, sourceImage.rows, CV_8UC1);
    fx.create(sourceImage.cols, sourceImage.rows, CV_8UC1);
    fy.create(sourceImage.cols, sourceImage.rows, CV_8UC1);

    grayProcess();
    Mat &gray = grayImage;
    for (int i = 1; i < gradImg.rows - 1; ++i) {
        for (int j = 1; j < gradImg.cols - 1; ++j) {
            fy.at<uchar>(i, j) = saturate_cast<uchar>(
                fabs(gray.at<uchar>(i + 1, j - 1)
                     + 2 * gray.at<uchar>(i + 1, j)
                     + gray.at<uchar>(i + 1, j + 1)
                     - gray.at<uchar>(i - 1, j - 1)
                     - 2 * gray.at<uchar>(i - 1, j)
                     - gray.at<uchar>(i - 1, j + 1))
                );

            fx.at<uchar>(i, j) = saturate_cast<uchar>(
                fabs(gray.at<uchar>(i - 1, j + 1)
                     + 2 * gray.at<uchar>(i, j + 1)
                     + gray.at<uchar>(i + 1, j + 1)
                     - gray.at<uchar>(i - 1, j - 1)
                     - 2 * gray.at<uchar>(i, j - 1)
                     - gray.at<uchar>(i + 1, j - 1))
                );

            gradImg.at<uchar>(i, j) = fx.at<uchar>(i, j) + fy.at<uchar>(i, j);

            grayImg.at<uchar>(i, j) = saturate_cast<uchar>(
                gray.at<uchar>(i, j) - gradImg.at<uchar>(i, j)
                );
        }
    }

    QImage qGradImg = matToQImage(gradImg);
    QImage qGrayImg = matToQImage(grayImg);
    imageSlotLabels[1]->setPixmap(QPixmap::fromImage(qGradImg));
    imageSlotLabels[2]->setPixmap(QPixmap::fromImage(qGrayImg));
}

void Opencv::prewittEdge() {
    Mat gradImg, grayImg, fx, fy;
    gradImg.create(sourceImage.cols, sourceImage.rows, CV_8UC1);
    grayImg.create(sourceImage.cols, sourceImage.rows, CV_8UC1);
    fx.create(sourceImage.cols, sourceImage.rows, CV_8UC1);
    fy.create(sourceImage.cols, sourceImage.rows, CV_8UC1);

    grayProcess();
    Mat &gray = grayImage;
    for (int i = 1; i < gradImg.rows - 1; ++i) {
        for (int j = 1; j < gradImg.cols - 1; ++j) {
            fx.at<uchar>(i, j) = saturate_cast<uchar>(
                fabs(gray.at<uchar>(i + 1, j - 1)
                     + gray.at<uchar>(i + 1, j)
                     + gray.at<uchar>(i + 1, j + 1)
                     - gray.at<uchar>(i - 1, j - 1)
                     - gray.at<uchar>(i - 1, j)
                     - gray.at<uchar>(i - 1, j + 1))
                );

            fy.at<uchar>(i, j) = saturate_cast<uchar>(
                fabs(gray.at<uchar>(i - 1, j + 1)
                     + gray.at<uchar>(i, j + 1)
                     + gray.at<uchar>(i + 1, j + 1)
                     - gray.at<uchar>(i - 1, j - 1)
                     - gray.at<uchar>(i, j - 1)
                     - gray.at<uchar>(i + 1, j - 1))
                );

            gradImg.at<uchar>(i, j) = max(fx.at<uchar>(i, j), fy.at<uchar>(i, j));

            grayImg.at<uchar>(i, j) = saturate_cast<uchar>(
                gray.at<uchar>(i, j) - gradImg.at<uchar>(i, j)
                );
        }
    }

    QImage qGradImg = matToQImage(gradImg);
    QImage qGrayImg = matToQImage(grayImg);
    imageSlotLabels[1]->setPixmap(QPixmap::fromImage(qGradImg));
    imageSlotLabels[2]->setPixmap(QPixmap::fromImage(qGrayImg));
}


// 双阈值连接
void Opencv::doubleThresholdLink(Mat &img, int lowTh, int highTh) {
    QQueue<cv::Point> queue;

    // 强边缘置255,非边缘置0，若边缘保留原值
    for (int i = 1; i < img.rows - 1; ++i) {
        for (int j = 1; j < img.cols - 1; ++j) {
            uchar &v = img.at<uchar>(i, j);
            if (v >= highTh) {
                v = 255;
                queue.push_back(cv::Point(j, i));
            } else if (v < lowTh) {
                v = 0;
            }
        }
    }

    // 遍历强边缘点，搜索连通路径
    while (!queue.empty()) {
        cv::Point p = queue.front();
        queue.pop_front();

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (dx == 0 && dy == 0)
                    continue;

                int x = p.x + dx;
                int y = p.y + dy;

                uchar &v = img.at<uchar>(x, y);
                // 与强边缘连接的弱边缘
                if (v >= lowTh && v < highTh) {
                    v = 255;
                    queue.push_back(cv::Point(x, y));
                }
            }
        }

    }

    for (int x = 0; x < img.rows; ++x) {
        for (int y = 0; y < img.cols; ++y) {
            // 删除所有没有与强边缘连接的弱边缘
            uchar &v = img.at<uchar>(x, y);
            if (v != 255)
                v = 0;
        }
    }
}

void Opencv::cannyEdge() {
    Mat gauss, fx, fy, gradImg, grayImg, directions;
    gauss.create(sourceImage.rows, sourceImage.cols, CV_8UC1);
    fx.create(sourceImage.rows, sourceImage.cols, CV_16SC1);
    fy.create(sourceImage.rows, sourceImage.cols, CV_16SC1);
    gradImg.create(sourceImage.rows, sourceImage.cols, CV_32FC1);
    grayImg.create(sourceImage.rows, sourceImage.cols, CV_8UC1);
    directions.create(sourceImage.rows, sourceImage.cols, CV_32FC1);

    grayProcess();
    Mat &gray = grayImage;

    // 高斯处理
    for (int i = 1; i < gauss.rows - 1; ++i) {
        for (int j = 1; j < gauss.cols - 1; ++j) {
            gauss.at<uchar>(i, j) = saturate_cast<uchar>(
                fabs(0.0751136 * gray.at<uchar>(i - 1, j - 1)
                     + 0.123841 * gray.at<uchar>(i - 1, j)
                     + 0.0751136 * gray.at<uchar>(i - 1, j + 1)
                     + 0.123841 * gray.at<uchar>(i, j - 1)
                     + 0.20418 * gray.at<uchar>(i, j)
                     + 0.123841 * gray.at<uchar>(i, j + 1)
                     + 0.0751136 * gray.at<uchar>(i + 1, j - 1)
                     + 0.123841 * gray.at<uchar>(i + 1, j)
                     + 0.0751136 * gray.at<uchar>(i + 1, j + 1)
                    )
                );
        }
    }

    // sobel处理
    for (int i = 2; i < gauss.rows - 2; ++i) {
        for (int j = 2; j < gauss.cols - 2; ++j) {
            fy.at<short>(i, j) =
                gauss.at<uchar>(i + 1, j - 1)
                 + 2 * gauss.at<uchar>(i + 1, j)
                 + gauss.at<uchar>(i + 1, j + 1)
                 - gauss.at<uchar>(i - 1, j - 1)
                 - 2 * gauss.at<uchar>(i - 1, j)
                 - gauss.at<uchar>(i - 1, j + 1);

            fx.at<short>(i, j) =
                gauss.at<uchar>(i - 1, j + 1)
                 + 2 * gauss.at<uchar>(i, j + 1)
                 + gauss.at<uchar>(i + 1, j + 1)
                 - gauss.at<uchar>(i - 1, j - 1)
                 - 2 * gauss.at<uchar>(i, j - 1)
                 - gauss.at<uchar>(i + 1, j - 1);

            // 获取梯度图
            gradImg.at<float>(i, j) = qSqrt(
                qPow(fx.at<short>(i, j), 2) + qPow(fy.at<short>(i, j), 2)
                );

            // 获取方向图
            directions.at<float>(i, j) = qAtan2(fy.at<short>(i, j), fx.at<short>(i, j));
        }
    }

    // 极大值抑制
    Mat nms(gradImg.size(), CV_32FC1, Scalar(0));
    for (int i = 3; i < gradImg.rows - 3; ++i) {
        for (int j = 3; j < gradImg.cols - 3; ++j) {
            float rad = directions.at<float>(i, j);

            // atan2 值域为 [-pi,pi]. 梯度方向正反等价
            if (rad < 0)
                rad += M_PI;

            float current = gradImg.at<float>(i, j);
            float g1, g2;

            if (rad < M_PI_4) {
                float weight = qTan(rad);
                // 右侧方向插值
                g1 = (1.0f - weight) * gradImg.at<float>(i, j + 1) +
                    weight * gradImg.at<float>(i - 1, j + 1);

                // 左侧方向插值
                g2 = (1.0f - weight) * gradImg.at<float>(i, j - 1) +
                    weight * gradImg.at<float>(i + 1, j - 1);
            }

            // 45° ~ 90°
            else if (rad < M_PI_2) {
                float weight = 1.0f / std::tan(rad);
                // 上侧方向插值
                g1 = (1.0f - weight) * gradImg.at<float>(i - 1, j) +
                    weight * gradImg.at<float>(i - 1, j + 1);

                // 下侧方向插值
                g2 = (1.0f - weight) * gradImg.at<float>(i + 1, j) +
                    weight * gradImg.at<float>(i + 1, j - 1);
            }

            // 90° ~ 135°
            else if (rad < M_PI * 3.0f / 4.0f) {
                // tan在这里为负，所以取负倒数
                float weight = -1.0f / std::tan(rad);

                // 上侧方向插值
                g1 = (1.0f - weight) * gradImg.at<float>(i - 1, j) +
                    weight * gradImg.at<float>(i - 1, j - 1);

                // 下侧方向插值
                g2 = (1.0f - weight) * gradImg.at<float>(i + 1, j) +
                    weight * gradImg.at<float>(i + 1, j + 1);
            }

            // 135° ~ 180°
            else {
                // tan为负，取负数得到0~1
                float weight = -std::tan(rad);

                // 左上方向
                g1 = (1.0f - weight) * gradImg.at<float>(i, j - 1) +
                    weight * gradImg.at<float>(i - 1, j - 1);

                // 右下方向
                g2 = (1.0f - weight) * gradImg.at<float>(i, j + 1) +
                    weight * gradImg.at<float>(i + 1, j + 1);
            }

            // 当前点必须是梯度方向上的局部最大值
            if (current >= g1 && current >= g2)
                nms.at<float>(i, j) = current;
        }
    }

    Mat edge;
    normalize(nms, edge, 0, 255, NORM_MINMAX, CV_8UC1);
    doubleThresholdLink(edge, 30, 60);

    for (int i = 0; i < edge.rows; ++i) {
        for (int j = 0; j < edge.cols; ++j) {
            grayImg.at<uchar>(i, j) = saturate_cast<uchar>(
                gray.at<uchar>(i, j) - edge.at<uchar>(i, j));
        }
    }

    QImage qEdgeImg = matToQImage(edge);
    QImage qGrayImg = matToQImage(grayImg);
    imageSlotLabels[1]->setPixmap(QPixmap::fromImage(qEdgeImg));
    imageSlotLabels[2]->setPixmap(QPixmap::fromImage(qGrayImg));
}

Mat Opencv::addSaltNoise(const Mat &src, int n) {
    Mat dst = src.clone();
    QRandomGenerator *gen = QRandomGenerator::global();

    // 盐噪声
    for (int k = 0; k < n; ++k) {
        int i = gen->generate() % dst.rows;
        int j = gen->generate() % dst.cols;

        if (dst.channels() == 1)
            dst.at<uchar>(i, j) = 255;
        else {
            dst.at<Vec3b>(i, j)[0] = 255;
            dst.at<Vec3b>(i, j)[1] = 255;
            dst.at<Vec3b>(i, j)[2] = 255;
        }
    }

    // 椒噪声
    for (int k = 0; k < n; ++k) {
        int i = gen->generate() % dst.rows;
        int j = gen->generate() % dst.cols;

        if (dst.channels() == 1)
            dst.at<uchar>(i, j) = 0;
        else {
            dst.at<Vec3b>(i, j)[0] = 0;
            dst.at<Vec3b>(i, j)[1] = 0;
            dst.at<Vec3b>(i, j)[2] = 0;
        }
    }

    return dst;
}

double Opencv::generateGaussianNoise(double mu, double sigma) {
    const double epsilon = std::numeric_limits<double>::min();
    double u1, u2;
    static double z0, z1;
    QRandomGenerator *gen = QRandomGenerator::global();

    do {
        u1 = 1.0 * gen->generate() / QRandomGenerator::max();
        u2 = 1.0 * gen->generate() / QRandomGenerator::max();
    } while (u1 <= epsilon);

    // Box-Muller公式
    z0 = qSqrt(-2 * std::log(u1)) * cos(2 * M_PI * u2);
    z1 = qSqrt(-2 * std::log(u1)) * sin(2 * M_PI * u2);
    Q_UNUSED(z1);
    return z0 * sigma + mu;
}

Mat Opencv::addGaussianNoise(const Mat &src) {
    Mat dst = src.clone();

    for (int i = 0; i < src.rows; ++i) {
        for (int j = 0; j < src.cols; ++j) {
            if (src.channels() == 1) {
                dst.at<uchar>(i, j) = saturate_cast<uchar>(
                    dst.at<uchar>(i, j) + generateGaussianNoise(2, 0.8) * 16);
            } else {
                dst.at<Vec3b>(i, j)[0] = saturate_cast<uchar>(
                    dst.at<Vec3b>(i, j)[0] + generateGaussianNoise(2, 0.8) * 16);
                dst.at<Vec3b>(i, j)[1] = saturate_cast<uchar>(
                    dst.at<Vec3b>(i, j)[1] + generateGaussianNoise(2, 0.8) * 16);
                dst.at<Vec3b>(i, j)[2] = saturate_cast<uchar>(
                    dst.at<Vec3b>(i, j)[2] + generateGaussianNoise(2, 0.8) * 16);
            }
        }
    }

    return dst;
}

void Opencv::saltNoise() {
    Mat salt;
    salt.create(sourceImage.rows, sourceImage.cols, CV_8UC1);
    salt = addSaltNoise(sourceImage, 800);

    // 保存噪声图像数据供别的模块使用
    noisedImage = salt.clone();

    QImage qSaltImg = matToQImage(salt);
    imageSlotLabels[0]->setPixmap(QPixmap::fromImage(qSaltImg));
}

void Opencv::gaussianNoise() {
    Mat gaussian;
    gaussian.create(sourceImage.rows, sourceImage.cols, CV_8UC1);
    gaussian = addGaussianNoise(sourceImage);

    noisedImage = gaussian.clone();

    QImage qGaussianImg = matToQImage(gaussian);
    imageSlotLabels[0]->setPixmap(QPixmap::fromImage(qGaussianImg));
}

void Opencv::averageFilter() {
    if (!noisePresent || noisedImage.empty())
        return;

    Mat filtered;
    filtered = noisedImage.clone();

    for (int i = 1; i < noisedImage.rows - 1; ++i) {
        for (int j = 1; j < noisedImage.cols - 1; ++j) {
            if (noisedImage.channels() == 1) {
                filtered.at<uchar>(i, j) =
                    saturate_cast<uchar>(
                        (noisedImage.at<uchar>(i - 1, j - 1)
                        + noisedImage.at<uchar>(i - 1, j)
                        + noisedImage.at<uchar>(i - 1, j + 1)
                        + noisedImage.at<uchar>(i, j - 1)
                        + noisedImage.at<uchar>(i, j)
                        + noisedImage.at<uchar>(i, j + 1)
                        + noisedImage.at<uchar>(i + 1, j - 1)
                        + noisedImage.at<uchar>(i + 1, j)
                        + noisedImage.at<uchar>(i + 1, j + 1)) / 9
                    );
            } else {
                for (int k = 0; k < 3; ++k) {
                    filtered.at<Vec3b>(i, j)[k] =
                        saturate_cast<uchar>(
                            (noisedImage.at<Vec3b>(i - 1, j - 1)[k]
                             + noisedImage.at<Vec3b>(i - 1, j)[k]
                             + noisedImage.at<Vec3b>(i - 1, j + 1)[k]
                             + noisedImage.at<Vec3b>(i, j - 1)[k]
                             + noisedImage.at<Vec3b>(i, j)[k]
                             + noisedImage.at<Vec3b>(i, j + 1)[k]
                             + noisedImage.at<Vec3b>(i + 1, j - 1)[k]
                             + noisedImage.at<Vec3b>(i + 1, j)[k]
                             + noisedImage.at<Vec3b>(i + 1, j + 1)[k]) / 9
                        );
                }
            }
        }
    }

    QImage qFilterdImg = matToQImage(filtered);
    imageSlotLabels[1]->setPixmap(QPixmap::fromImage(qFilterdImg));
}

void Opencv::middleFilter() {
    if (!noisePresent || noisedImage.empty())
        return;

    Mat filtered;
    filtered = noisedImage.clone();

    std::multiset<uchar>pixels;
    QVector<QVector<int>>neighbors = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},  {0, 0},  {0, 1},
        {1, -1},  {1, 0},  {1, 1},
    };

    for (int i = 1; i < noisedImage.rows - 1; ++i) {
        for (int j = 1; j < noisedImage.cols - 1; ++j) {
            if (noisedImage.channels() == 1) {
                pixels.clear();

                for (const auto &it : neighbors) {
                    pixels.insert(noisedImage.at<uchar>(i + it[0], j + it[1]));
                }

                auto it = pixels.begin();
                std::advance(it, pixels.size() / 2);
                filtered.at<uchar>(i, j) = *it;
            } else {
                for (int k = 0; k < 3; ++k) {
                    pixels.clear();

                    for (const auto &it : neighbors) {
                        pixels.insert(noisedImage.at<Vec3b>(i + it[0], j + it[1])[k]);
                    }

                    auto it = pixels.begin();
                    std::advance(it, pixels.size() / 2);
                    filtered.at<Vec3b>(i, j)[k] = *it;
                }
            }
        }
    }

    QImage qFilterdImg = matToQImage(filtered);
    imageSlotLabels[1]->setPixmap(QPixmap::fromImage(qFilterdImg));
}

void Opencv::windowFilter() {
    if (!noisePresent || noisedImage.empty())
        return;

    Mat filtered;
    filtered = noisedImage.clone();

    QVector<uchar> window(8,0);
    QVector<float>minImg(8,0);

    for (int i = 1; i < noisedImage.rows - 1; ++i) {
        for (int j = 1; j < noisedImage.cols - 1; ++j) {
            if (noisedImage.channels() == 1) {
                window[0] = (noisedImage.at<uchar>(i - 1, j - 1)
                             + noisedImage.at<uchar>(i - 1, j)
                             + noisedImage.at<uchar>(i, j - 1)
                             + noisedImage.at<uchar>(i, j)) / 4;
                window[1] = (noisedImage.at<uchar>(i - 1, j)
                             + noisedImage.at<uchar>(i - 1, j + 1)
                             + noisedImage.at<uchar>(i, j)
                             + noisedImage.at<uchar>(i, j + 1)) / 4;
                window[2] = (noisedImage.at<uchar>(i, j - 1)
                             + noisedImage.at<uchar>(i, j)
                             + noisedImage.at<uchar>(i + 1, j - 1)
                             + noisedImage.at<uchar>(i + 1, j)) / 4;
                window[3] = (noisedImage.at<uchar>(i, j)
                             + noisedImage.at<uchar>(i, j + 1)
                             + noisedImage.at<uchar>(i + 1, j)
                             + noisedImage.at<uchar>(i + 1, j + 1)) / 4;
                window[4] = (noisedImage.at<uchar>(i - 1, j - 1)
                             + noisedImage.at<uchar>(i - 1, j)
                             + noisedImage.at<uchar>(i - 1, j + 1)
                             + noisedImage.at<uchar>(i, j - 1)
                             + noisedImage.at<uchar>(i, j)
                             + noisedImage.at<uchar>(i, j + 1)) / 6;
                window[5] = (noisedImage.at<uchar>(i, j - 1)
                             + noisedImage.at<uchar>(i, j)
                             + noisedImage.at<uchar>(i, j + 1)
                             + noisedImage.at<uchar>(i + 1, j - 1)
                             + noisedImage.at<uchar>(i + 1, j)
                             + noisedImage.at<uchar>(i + 1, j + 1)) / 6;
                window[6] = (noisedImage.at<uchar>(i - 1, j)
                             + noisedImage.at<uchar>(i - 1, j + 1)
                             + noisedImage.at<uchar>(i, j)
                             + noisedImage.at<uchar>(i, j + 1)
                             + noisedImage.at<uchar>(i + 1, j)
                             + noisedImage.at<uchar>(i + 1, j + 1)) / 6;
                window[7] = (noisedImage.at<uchar>(i - 1, j - 1)
                             + noisedImage.at<uchar>(i - 1, j)
                             + noisedImage.at<uchar>(i, j)
                             + noisedImage.at<uchar>(i, j - 1)
                             + noisedImage.at<uchar>(i + 1, j)
                             + noisedImage.at<uchar>(i + 1, j - 1)) / 6;

                for (int n = 0; n < window.size(); ++n) {
                    minImg[n] = qPow(window[n] - noisedImage.at<uchar>(i, j), 2);
                }
                auto smallest = std::min_element(minImg.begin(), minImg.end());
                int pos = std::distance(minImg.begin(), smallest);
                filtered.at<uchar>(i, j) = saturate_cast<uchar>(window[pos]);
            } else {
                for (int k = 0; k < 3; ++k) {
                    window[0] = (noisedImage.at<Vec3b>(i - 1, j - 1)[k]
                                 + noisedImage.at<Vec3b>(i - 1, j)[k]
                                 + noisedImage.at<Vec3b>(i, j - 1)[k]
                                 + noisedImage.at<Vec3b>(i, j)[k]) / 4;
                    window[1] = (noisedImage.at<Vec3b>(i - 1, j)[k]
                                 + noisedImage.at<Vec3b>(i - 1, j + 1)[k]
                                 + noisedImage.at<Vec3b>(i, j)[k]
                                 + noisedImage.at<Vec3b>(i, j + 1)[k]) / 4;
                    window[2] = (noisedImage.at<Vec3b>(i, j - 1)[k]
                                 + noisedImage.at<Vec3b>(i, j)[k]
                                 + noisedImage.at<Vec3b>(i + 1, j - 1)[k]
                                 + noisedImage.at<Vec3b>(i + 1, j)[k]) / 4;
                    window[3] = (noisedImage.at<Vec3b>(i, j)[k]
                                 + noisedImage.at<Vec3b>(i, j + 1)[k]
                                 + noisedImage.at<Vec3b>(i + 1, j)[k]
                                 + noisedImage.at<Vec3b>(i + 1, j + 1)[k]) / 4;
                    window[4] = (noisedImage.at<Vec3b>(i - 1, j - 1)[k]
                                 + noisedImage.at<Vec3b>(i - 1, j)[k]
                                 + noisedImage.at<Vec3b>(i - 1, j + 1)[k]
                                 + noisedImage.at<Vec3b>(i, j - 1)[k]
                                 + noisedImage.at<Vec3b>(i, j)[k]
                                 + noisedImage.at<Vec3b>(i, j + 1)[k]) / 6;
                    window[5] = (noisedImage.at<Vec3b>(i, j - 1)[k]
                                 + noisedImage.at<Vec3b>(i, j)[k]
                                 + noisedImage.at<Vec3b>(i, j + 1)[k]
                                 + noisedImage.at<Vec3b>(i + 1, j - 1)[k]
                                 + noisedImage.at<Vec3b>(i + 1, j)[k]
                                 + noisedImage.at<Vec3b>(i + 1, j + 1)[k]) / 6;
                    window[6] = (noisedImage.at<Vec3b>(i - 1, j)[k]
                                 + noisedImage.at<Vec3b>(i - 1, j + 1)[k]
                                 + noisedImage.at<Vec3b>(i, j)[k]
                                 + noisedImage.at<Vec3b>(i, j + 1)[k]
                                 + noisedImage.at<Vec3b>(i + 1, j)[k]
                                 + noisedImage.at<Vec3b>(i + 1, j + 1)[k]) / 6;
                    window[7] = (noisedImage.at<Vec3b>(i - 1, j - 1)[k]
                                 + noisedImage.at<Vec3b>(i - 1, j)[k]
                                 + noisedImage.at<Vec3b>(i, j)[k]
                                 + noisedImage.at<Vec3b>(i, j - 1)[k]
                                 + noisedImage.at<Vec3b>(i + 1, j)[k]
                                 + noisedImage.at<Vec3b>(i + 1, j - 1)[k]) / 6;

                    for (int n = 0; n < window.size(); ++n) {
                        minImg[n] = qPow(window[n] - noisedImage.at<Vec3b>(i, j)[k], 2);
                    }
                    auto smallest = std::min_element(minImg.begin(), minImg.end());
                    int pos = std::distance(minImg.begin(), smallest);
                    filtered.at<Vec3b>(i, j)[k] = saturate_cast<uchar>(window[pos]);
                }
            }
        }
    }

    QImage qFilterdImg = matToQImage(filtered);
    imageSlotLabels[1]->setPixmap(QPixmap::fromImage(qFilterdImg));
}

void Opencv::gaussFilter() {
    if (!noisePresent || noisedImage.empty())
        return;

    Mat filtered;
    filtered = noisedImage.clone();

    for (int i = 1; i < noisedImage.rows - 1; ++i) {
        for (int j = 1; j < noisedImage.cols - 1; ++j) {
            if (noisedImage.channels() == 1) {
                filtered.at<uchar>(i, j) = saturate_cast<uchar>(
                        (noisedImage.at<uchar>(i - 1, j - 1)
                        + 2 * noisedImage.at<uchar>(i - 1, j)
                        + noisedImage.at<uchar>(i - 1, j + 1)
                        + 2 * noisedImage.at<uchar>(i, j - 1)
                        + 4 * noisedImage.at<uchar>(i, j)
                        + 2 * noisedImage.at<uchar>(i, j + 1)
                        + noisedImage.at<uchar>(i + 1, j - 1)
                        + 2 * noisedImage.at<uchar>(i + 1, j)
                        + noisedImage.at<uchar>(i + 1, j + 1)
                        ) / 16
                );
            } else {
                for (int k = 0; k < 3; ++k) {
                    filtered.at<Vec3b>(i, j)[k] = saturate_cast<uchar>(
                            (noisedImage.at<Vec3b>(i - 1, j - 1)[k]
                             + 2 * noisedImage.at<Vec3b>(i - 1, j)[k]
                             + noisedImage.at<Vec3b>(i - 1, j + 1)[k]
                             + 2 * noisedImage.at<Vec3b>(i, j - 1)[k]
                             + 4 * noisedImage.at<Vec3b>(i, j)[k]
                             + 2 * noisedImage.at<Vec3b>(i, j + 1)[k]
                             + noisedImage.at<Vec3b>(i + 1, j - 1)[k]
                             + 2 * noisedImage.at<Vec3b>(i + 1, j)[k]
                             + noisedImage.at<Vec3b>(i + 1, j + 1)[k]
                             ) / 16
                    );
                }
            }
        }
    }

    QImage qFilterdImg = matToQImage(filtered);
    imageSlotLabels[1]->setPixmap(QPixmap::fromImage(qFilterdImg));
}

void Opencv::morphFilter() {
    Mat open, close, temp, element;

    // 15x15卷积核
    element = getStructuringElement(MORPH_RECT, Size(3, 3));
    // 开运算
    erode(sourceImage, temp, element);
    dilate(temp, open, element);
    // 闭运算
    dilate(sourceImage, temp, element);
    erode(temp, close, element);

    QImage qOpen = matToQImage(open);
    QImage qClose = matToQImage(close);
    imageSlotLabels[1]->setPixmap(QPixmap::fromImage(qOpen));
    imageSlotLabels[2]->setPixmap(QPixmap::fromImage(qClose));
}

void Opencv::affineTransform() {
    Point2f srcTri[3], dstTri[3];
    Mat rotMat(2, 3, CV_32FC1);
    Mat warpMat(2, 3, CV_32FC1);

    // 取原图的三个点
    srcTri[0] = Point2f(0, 0);
    srcTri[1] = Point2f(sourceImage.cols - 1, 0);
    srcTri[2] = Point2f(0, sourceImage.rows - 1);

    // 假设原图的三个点转换后所处坐标
    dstTri[0] = Point2f(sourceImage.cols * 0, sourceImage.rows * 0.33);
    dstTri[1] = Point2f(sourceImage.cols * 0.85, sourceImage.rows * 0.25);
    dstTri[2] = Point2f(sourceImage.cols * 0.15, sourceImage.rows * 0.7);

    // 根据上面的关系计算整体转换矩阵
    Mat dst(sourceImage.rows, sourceImage.cols, sourceImage.type());
    warpMat = getAffineTransform(srcTri, dstTri);
    warpAffine(sourceImage, dst, warpMat, sourceImage.size());

    QImage qDst = matToQImage(dst);
    imageSlotLabels[0]->setPixmap(QPixmap::fromImage(qDst));
}

void Opencv::perspectiveTransform() {
    Point2f srcQuad[4], dstQuad[4];
    Mat warpMat(3, 3, CV_32FC1);

    srcQuad[0] = Point2f(0, 0);
    srcQuad[1] = Point2f(sourceImage.cols - 1, 0);
    srcQuad[2] = Point2f(0, sourceImage.rows - 1);
    srcQuad[3] = Point2f(sourceImage.cols - 1, sourceImage.rows - 1);

    dstQuad[0] = Point2f(sourceImage.cols * 0.05, sourceImage.rows * 0.33);
    dstQuad[1] = Point2f(sourceImage.cols * 0.9, sourceImage.rows * 0.25);
    dstQuad[2] = Point2f(sourceImage.cols * 0.2, sourceImage.rows * 0.7);
    dstQuad[3] = Point2f(sourceImage.cols * 0.8, sourceImage.rows * 0.9);

    Mat dst(sourceImage.rows, sourceImage.cols, sourceImage.type());
    warpMat = getPerspectiveTransform(srcQuad, dstQuad);
    warpPerspective(sourceImage, dst, warpMat, sourceImage.size());

    QImage qDst = matToQImage(dst);
    imageSlotLabels[0]->setPixmap(QPixmap::fromImage(qDst));
}

void Opencv::thresholdSeg() {
    // 获取灰度图
    grayProcess();

    Mat target(grayImage.size(), grayImage.type());

    for (int i = 0; i < grayImage.rows; ++i) {
        for (int j = 0; j < grayImage.cols; ++j) {
            if (grayImage.at<uchar>(i, j) > 100)
                target.at<uchar>(i, j) = 255;
            else
                target.at<uchar>(i, j) = 0;
        }
    }

    QImage qTarget = matToQImage(target);
    imageSlotLabels[1]->setPixmap(QPixmap::fromImage(qTarget));
}

int Opencv::otsu(const QVector<int> &hist) {
    float u0, u1, w0, w1;
    int count0, maxT;
    float devi, maxDevi = 0;
    int i, sum = 0;

    sum = std::accumulate(hist.begin(), hist.end(), 0);

    for (int t = 0; t < 255; ++t) {
        u0 = count0 = 0;
        // 阈值为t时，c0组的均值及产生的概率
        for (int i = 0; i < t; ++i) {
            u0 += i * hist[i];
            count0 += hist[i];
        }
        u0 /= count0;
        w0 = 1.0 * count0 / sum;

        u1 = 0;
        // 阈值为t时，c1组的均值及产生的概率
        for (i = t + 1; i < 256; i++) {
            u1 += i * hist[i];
        }
        u1 /= (sum - count0);
        w1 = 1 - w0;
        devi = w0 * w1 * (u1 - u0) * (u1 - u0);
        if (devi > maxDevi) {
            maxDevi = devi;
            maxT = t;
        }
    }
    return maxT;
}

void Opencv::otsuSeg() {
    grayProcess();

    QVector<int> hist(256, 0);
    for (int i = 0; i < grayImage.rows; ++i) {
        for (int j = 0; j < grayImage.cols; ++j) {
            hist[grayImage.at<uchar>(i, j)]++;
        }
    }

    int T = otsu(hist);

    Mat target(grayImage.size(), grayImage.type());
    for (int i = 0; i < grayImage.rows; ++i) {
        for (int j = 0; j < grayImage.cols; ++j) {
            if (grayImage.at<uchar>(i, j) > T)
                target.at<uchar>(i, j) = 255;
            else
                target.at<uchar>(i, j) = 0;
        }
    }

    QImage qTarget = matToQImage(target);
    imageSlotLabels[1]->setPixmap(QPixmap::fromImage(qTarget));
}

void Opencv::kittlerSeg() {
    grayProcess();

    Mat target(grayImage.size(), grayImage.type());
    int grads, sumGrads = 0, sumGrayGrads = 0, KT;

    for (int i = 1; i < grayImage.rows - 1; ++i) {
        uchar *prev = grayImage.ptr(i - 1);
        uchar *curr = grayImage.ptr(i);
        uchar *next = grayImage.ptr(i + 1);
        for (int j = 1; j < grayImage.cols - 1; ++j) {
            // 求水平或垂直方向的最大梯度
            grads = qMax(qAbs(prev[j] - next[j]), qAbs(curr[j - 1] - curr[j + 1]));
            sumGrads += grads;
            sumGrayGrads += grads * curr[j];
        }
    }

    if (sumGrads == 0) {
        qDebug() << "[Opencv] Error: divided by 0 happened in kittlerSeg()";
        return;
    }

    KT = sumGrayGrads / sumGrads;
    for (int i = 0; i < grayImage.rows; ++i) {
        for (int j = 0; j < grayImage.cols; ++j) {
            if (grayImage.at<uchar>(i, j) > KT)
                target.at<uchar>(i, j) = 255;
            else
                target.at<uchar>(i, j) = 0;
        }
    }

    QImage qTarget = matToQImage(target);
    imageSlotLabels[1]->setPixmap(QPixmap::fromImage(qTarget));
}
