#ifndef OPENCV_H
#define OPENCV_H

#include <QWidget>
#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QListWidget>
#include <opencv4/opencv2/core/core.hpp>
#include <opencv4/opencv2/imgcodecs.hpp>
#include <opencv4/opencv2/imgproc.hpp>

#define DEFAULT_SOURCE_IMAGE    ":/banna/image/ren.jpg"

class FuncPanel: public QWidget
{
    Q_OBJECT
public:
    FuncPanel(const QString &title, const QStringList &items, QWidget *parent = nullptr);
    ~FuncPanel();

    QLabel *title;
    QListWidget *listWidget;
    QVBoxLayout *layout;
};

class Opencv: public QWidget
{
    Q_OBJECT
public:
    Opencv(QWidget *parent = nullptr);
    ~Opencv();

    static bool noisePresent;
    static QImage matToQImage(const cv::Mat &mat);
private:
    // 原图显示标签
    QLabel *sourceImageLabel;

    // 处理图像槽，槽位0用于显示灰度图或噪声图
    QLabel *imageSlotLabels[3];

    // 图片选择按钮
    QPushButton *selectButton;

    // 控制相关控件
    FuncPanel *procPanel;
    FuncPanel *filterPanel;
    FuncPanel *distinctPanel;
    FuncPanel *noisePanel;
    FuncPanel *cameraPanel;
    FuncPanel *transformPanel;
    FuncPanel *edgePanel;
    FuncPanel *SegmentationPanel;
    FuncPanel *characterPanel;

    // 主界面布局
    QHBoxLayout *hMainLayout;

    // 显示界面布局
    QGridLayout *gDisplayLayout;

    // 控制面板布局
    QGridLayout *gControlLayout;

    cv::Mat sourceImage;
    cv::Mat grayImage;
    cv::Mat noisedImage;

    void layoutInit();
    void slotImagesClear();
    void clearGrayImage();
    void clearNoisedImage();
    void grayProcess();

    // 灰度处理相关
    cv::Mat getGrayLevel(const cv::Mat &gray);
    void grayHistogram();
    void grayBalance();

    // 边缘相关
    void laplaceSharpen();
    void robertsEdge();
    void sobelEdge();
    void prewittEdge();
    void doubleThresholdLink(cv::Mat &img, int lowTh, int highTh);
    void cannyEdge();

    // 噪声相关
    cv::Mat addSaltNoise(const cv::Mat &src, int n);
    double generateGaussianNoise(double mu, double sigma);
    cv::Mat addGaussianNoise(const cv::Mat &src);
    void saltNoise();
    void gaussianNoise();

    // 滤波相关
    void averageFilter();
    void middleFilter();
    void windowFilter();
    void gaussFilter();
    void morphFilter();

    // 变换相关
    void affineTransform();
    void perspectiveTransform();

    // 分割相关
    void thresholdSeg();
    int otsu(const QVector<int> &hist);
    void otsuSeg();
    void kittlerSeg();

private slots:
    void selectButtonClicked();
    void procPanelClicked(QListWidgetItem *item);
    void filterPanelClicked(QListWidgetItem *item);
    void distinctPanelClicked(QListWidgetItem *item);
    void noisePanelClicked(QListWidgetItem *item);
    void cameraPanelClicked(QListWidgetItem *item);
    void transformPanelClicked(QListWidgetItem *item);
    void edgePanelClicked(QListWidgetItem *item);
    void SegmentationPanelClicked(QListWidgetItem *item);
    void characterPanelClicked(QListWidgetItem *item);
};

#endif // OPENCV_H
