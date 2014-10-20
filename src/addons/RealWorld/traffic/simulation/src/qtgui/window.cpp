#ifdef WITH_GUI

#include <QtGui>

#include "../TrafficSimulator.h"
#include "renderarea.h"
#include "window.h"

Window::Window(TrafficSimulator *simulator) {
    resize(WIN_X, WIN_Y);
    this->move(QApplication::desktop()->availableGeometry().center() - this->rect().center());

    renderArea = new RenderArea(simulator);

    QGridLayout *mainLayout = new QGridLayout;

    headerLabel = new QLabel(tr("QtGui V2.0"));
    headerLabel->setFont(QFont(headerLabel->font().family(), 20));

    mesoTypeComboBox = new QComboBox;
    mesoTypeComboBox->addItem(tr("Street Type"), RenderArea::Type);
    mesoTypeComboBox->addItem(tr("Maximum Speed"), RenderArea::MaxSpeed);
    mesoTypeComboBox->addItem(tr("Relative Vehicle Count"), RenderArea::RelativeVehicleCount);
    mesoTypeComboBox->addItem(tr("Direction (Forward=green)"), RenderArea::Direction);
    mesoTypeComboBox->addItem(tr("None"), RenderArea::None);
    mesoTypeComboBox->setCurrentIndex(2);
    mesoTypeLabel = new QLabel(tr("&Meso display:"));
    mesoTypeLabel->setBuddy(mesoTypeComboBox);

    mesoLabelCheckBox = new QCheckBox("&Label on meso-streets");
    mesoLabelCheckBox->setChecked(true);

    nodeCountLabel = new QLabel(tr("#Crossroads: ???"));
    streetCountLabel = new QLabel(tr("#Streets: ???"));
    microVehicleCountLabel = new QLabel(tr("#Micro-Vehicles: ???"));
    mesoVehicleCountLabel = new QLabel(tr("#Meso-Vehicles: ???"));
    offmapVehicleCountLabel = new QLabel(tr("#Offmap-Vehicles: ???"));
    sumVehicleCountLabel = new QLabel(tr("Sum Vehicles: ???"));

    nodeSearchLabel = new QLabel(tr("Search element (Prefixes n, s, v)"));
    nodeSearchBox = new QLineEdit();
    nodeSearchBox->setMaximumWidth(300);

    dataLabel = new QLabel(tr(""));
    dataLabel->setWordWrap(true);
    dataLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    startStopButton = new QPushButton("Running");
    startStopButton->setCheckable(true);
    startStopButton->setChecked(false);
    singleStepCheckBox = new QCheckBox("Pseudo-&Single step");

    mainLayout->setColumnMinimumWidth(0, 450);

    mainLayout->addWidget(renderArea, 0, 1, 20, 12);

    int row = 0;
    mainLayout->addWidget(headerLabel, row++, 0);

    mainLayout->setRowMinimumHeight(row++, 30);

    mainLayout->addWidget(startStopButton, row++, 0);
    mainLayout->addWidget(singleStepCheckBox, row++, 0);

    mainLayout->addWidget(mesoTypeLabel, row++, 0);
    mainLayout->addWidget(mesoTypeComboBox, row++, 0);
    mainLayout->addWidget(mesoLabelCheckBox, row++, 0);

    mainLayout->setRowMinimumHeight(row++, 30);
    mainLayout->addWidget(nodeCountLabel, row++, 0);
    mainLayout->addWidget(streetCountLabel, row++, 0);
    mainLayout->addWidget(microVehicleCountLabel, row++, 0);
    mainLayout->addWidget(mesoVehicleCountLabel, row++, 0);
    mainLayout->addWidget(offmapVehicleCountLabel, row++, 0);
    mainLayout->addWidget(sumVehicleCountLabel, row++, 0);

    mainLayout->setRowMinimumHeight(row++, 60);
    mainLayout->addWidget(nodeSearchLabel, row++, 0);
    mainLayout->addWidget(nodeSearchBox, row++, 0);
    mainLayout->addWidget(dataLabel, row++, 0);


    mainLayout->setRowMinimumHeight(row++, 60);

    setLayout(mainLayout);

    setWindowTitle(tr("TrafficSimulator - QtGui"));

    showMaximized();
}

#endif // WITH_GUI
