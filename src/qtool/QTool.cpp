
#include "QTool.h"
#include <iostream>

namespace qtool {

using data::DataManager;
using data::DataLoader;
using colorcreator::ColorCreator;
using viewer::MemoryViewer;
using viewer::BallEKFViewer;
using viewer::FieldViewer;
using offline::OfflineViewer;

QTool::QTool() : QMainWindow(),
        toolTabs(new QTabWidget()),
<<<<<<< HEAD
 	colorScrollArea(new QScrollArea()),
=======
        colorScrollArea(new QScrollArea()),
>>>>>>> newBots
        dataManager(new DataManager()),
        dataLoader(new DataLoader(dataManager)),
        colorCreator(new ColorCreator(dataManager)),
        memoryViewer(new MemoryViewer(dataManager->getMemory())),
        offlineViewer(new OfflineViewer(dataManager->getMemory())),
        ballEKFViewer(new BallEKFViewer(dataManager)),
        fieldViewer(new FieldViewer(dataManager)){

<<<<<<< HEAD
    this->setWindowTitle(tr("The New Tool of Awesome"));

    toolbar = new QToolBar();
    nextButton = new QPushButton(tr("&Next"));
    prevButton = new QPushButton(tr("&Previous"));

    connect(nextButton, SIGNAL(clicked()), this, SLOT(next()));
    connect(prevButton, SIGNAL(clicked()), this, SLOT(prev()));

    toolbar->addWidget(prevButton);
    toolbar->addWidget(nextButton);

    this->addToolBar(toolbar);
=======
    this->setWindowTitle(tr("QTool"));
>>>>>>> newBots

    this->setCentralWidget(toolTabs);

    colorScrollArea->setWidget(colorCreator);

    toolTabs->addTab(colorScrollArea, tr("Color Creator"));
    toolTabs->addTab(dataLoader, tr("Data Loader"));
    toolTabs->addTab(memoryViewer, tr("Log Viewer"));
    toolTabs->addTab(offlineViewer, tr("Offline Viewer"));
    toolTabs->addTab(ballEKFViewer, tr("BallEKF Viewer"));
    toolTabs->addTab(fieldViewer, tr("Field Viewer"));

    dataManager->addSubscriber(colorCreator, man::memory::MIMAGE_ID);
}

QTool::~QTool() {
    delete colorCreator;
    delete dataLoader;
    delete toolTabs;
}

void QTool::next() {
    dataManager->getNext();
}

void QTool::prev() {
    dataManager->getPrev();
}

}
