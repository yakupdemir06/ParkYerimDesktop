#ifndef THREADMANAGER_HPP
#define THREADMANAGER_HPP

#include <QObject>


#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

class CameraStream;
class ImageProcess;
class ApplicationWindow;


class ThreadManager : public QObject
{
    Q_OBJECT
public:

    ~ThreadManager();
    static ThreadManager* getInstance(ApplicationWindow* appwindow, unsigned int vehicle_in_CamIndex, unsigned int vehicle_out_CamIndex);

signals:
    void startThreads();
    void stopThreads();
    void terminateThreads();

private slots:
    void stopAllThreads();
    void terminateAllThreads();
    void recognizePlate_in();
    void recognizePlate_out();


private:
    ThreadManager(ApplicationWindow* appwindow, unsigned int vehicle_in_CamIndex, unsigned int vehicle_out_CamIndex);

    ApplicationWindow* m_appwindow = nullptr;
    CameraStream* m_camVehicleIn = nullptr;
    CameraStream* m_camVehicleOut = nullptr;
    ImageProcess* m_plateReaderVehicleIn = nullptr;
    ImageProcess* m_plateReaderVehicleOut = nullptr;
    static ThreadManager* m_instance;


};

#endif // THREADMANAGER_HPP
