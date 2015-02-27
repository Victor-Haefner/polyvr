#ifdef WITH_GUI

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;

#define WIN_X 1920
#define WIN_Y 1080

class RenderArea;
class TrafficSimulator;

/// A class that creates a QT window.
class Window : public QWidget {
//     Q_OBJECT

    public:
        /**
         The constructor.
         @param roadSystem The RoadSystem to retrieve data to display from.
         */
        Window(TrafficSimulator *simulator);

    private:

        friend class RenderArea;

        /// The area the map will be rendered in.
        RenderArea *renderArea;

        /// A header label.
        QLabel *headerLabel;

        //QLabel *drawTypeLabel;
        //QComboBox *drawTypeComboBox;
        /// A label for the type-choice.
        QLabel *mesoTypeLabel;
        /// The choice of the drawing type.
        QComboBox *mesoTypeComboBox;
        /// A checkbox to toggle labels for meso streets.
        QCheckBox *mesoLabelCheckBox;

        /// A label that will display the number of nodes.
        QLabel *nodeCountLabel;
        /// A label that will display the number of streets.
        QLabel *streetCountLabel;
        /// A label that will display the number of micro-vehicles.
        QLabel *microVehicleCountLabel;
        /// A label that will display the number of meso-vehicles.
        QLabel *mesoVehicleCountLabel;
        /// A label that will display the number of vehicles currently not on the map.
        QLabel *offmapVehicleCountLabel;
        /// A label that will display the number of vehicles currently not on the map.
        QLabel *sumVehicleCountLabel;

        /// A label that to describe the search box.
        QLabel *nodeSearchLabel;
        /// A search box.
        QLineEdit *nodeSearchBox;

        /// A label to display information about the selected object.
        QLabel *dataLabel;

        /// A button to start && pause the simulator
        QPushButton *startStopButton;
        /// A checkbox to use single-step simulation.
        QCheckBox *singleStepCheckBox;
};

#endif // WINDOW_H

#endif // WITH_GUI
