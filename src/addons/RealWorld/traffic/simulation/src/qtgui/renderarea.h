#ifdef WITH_GUI

#ifndef RENDERAREA_H
#define RENDERAREA_H

#include <QBrush>
#include <QPen>
#include <QPixmap>
#include <QWidget>

#include <string>

using namespace std;

#include "../types.h"

class Node;
class NodeLogic;
class Street;
class Vehicle;
class TrafficSimulator;
class RoadSystem;

/// The component of the window the street map will be rendered in.
class RenderArea : public QWidget {
    // Q_OBJECT

    private:
        /**
         Applies zoom && position.
         @param p The point to transform.
         @return The transformed point.
         */
        QPointF transform(QPointF p);

        /**
         Applies zoom.
         @param n A value to scale.
         @return The zoomed variable.
         */
        double scale(double n);

        /// The simulator that contains the RoadSystem.
        TrafficSimulator *simulator;

        /// The road system to get the data from.
        RoadSystem *roadSystem;

        /**
         Draws a line.
         @param painter The painter-object to use for drawing.
         @param start The start position.
         @param end The end position.
         @param capMaxWidth If set to \c false the line is allowed to become bigger than the given size through zooming.
         */
        void drawLine(QPainter& painter, Vec2f start, Vec2f end, bool capMaxWidth = true);

        /**
         Draws a line.
         @param painter The painter-object to use for drawing.
         @param start The start position.
         @param end The end position.
         @param capMaxWidth If set to \c false the line is allowed to become bigger than the given size through zooming.
         */
        void drawLine(QPainter& painter, QPointF start, QPointF end, bool capMaxWidth = true);

        /**
         Draws a circle.
         @param painter The painter-object to use for drawing.
         @param pos The position of the center.
         @param radius The radius.
         */
        void drawEllipse(QPainter& painter, Vec2f pos, double radius);

        /**
         Draws a circle.
         @param painter The painter-object to use for drawing.
         @param pos The position of the center.
         @param radius The radius.
         */
        void drawEllipse(QPainter& painter, QPointF pos, double radius);

        /**
         Draws a square.
         @param painter The painter-object to use for drawing.
         @param pos The position of the center.
         @param size The side length.
         */
        void drawRect(QPainter& painter, Vec2f pos, double size);

        /**
         Draws a square.
         @param painter The painter-object to use for drawing.
         @param pos The position of the center.
         @param size The side length.
         */
        void drawRect(QPainter& painter, QPointF pos, double size);

        /**
         Draws a text.
         @param painter The painter-object to use for drawing.
         @param pos The position.
         @param text The text to write.
         */
        void drawText(QPainter& painter, Vec2f pos, string text);

        /**
         Draws a text.
         @param painter The painter-object to use for drawing.
         @param pos The position.
         @param text The text to write.
         */
        void drawText(QPainter& painter, QPointF pos, QString text);


        /**
         Draws a node of the street map.
         @param painter The painter-object to use for drawing.
         @param node The node to draw.
         */
        void drawNode(QPainter& painter, Node *node);

        /**
         Draws a street of the street map.
         @param painter The painter-object to use for drawing.
         @param street The street to draw.
         */
        void drawStreet(QPainter& painter, Street *street);

        /**
         Draws a vehicle of the street map.
         @param painter The painter-object to use for drawing.
         @param vehicle The vehicle to draw.
         */
        void drawVehicle(QPainter& painter, Vehicle *vehicle);


        /**
         Is called by the Qt-system if the event occurs.
         @param event Data about the event.
         */
        void paintEvent(QPaintEvent *event);

        /**
         Is called by the Qt-system if the event occurs.
         @param event Data about the event.
         */
        void mousePressEvent(QMouseEvent *event);

        /**
         Is called by the Qt-system if the event occurs.
         @param event Data about the event.
         */
        void mouseMoveEvent(QMouseEvent *event);

        /**
         Is called by the Qt-system if the event occurs.
         @param event Data about the event.
         */
        void mouseReleaseEvent(QMouseEvent *event);

        /**
         Is called by the Qt-system if the event occurs.
         @param event Data about the event.
         */
        void wheelEvent(QWheelEvent *event);

        /// The position of the last mouse-move event.
        QPoint mouse_last;
        /// The position where the mouse has been pressed down.
        QPoint mouse_down;

        /// The current zoom level.
        double zoom;
        /// The current panning position of the map.
        QPoint position;

        /// The last entry in the search box.
        /// If the current entry is not the same as this one, it will be processed.
        QString lastShow;

        /// Selected vehicle object.
        unsigned int nearestVehicle;
        /// Selected node object.
        Node *nearestNode;
        /// Selected street object.
        Street *nearestStreet;
        /// Selected node logic object.
        NodeLogic *nearestNodeLogic;

        /// The previous state of the start/pause button
        bool lastPauseButtonChecked;

    public:

        /// The values for the drop-down box.
        enum MesoType {
            /// Display the maximum speed.
            MaxSpeed,
            /// Display the vehicle count.
            RelativeVehicleCount,
            /// Display the type.
            Type,
            /// Display the direction of the lanes.
            Direction,
            /// Display no special information.
            None
        };

        /**
         Creates an object of this class.
         @param roadSystem The RoadSystem to work on.
         @param parent The window this widget is part of.
         */
        RenderArea(TrafficSimulator *simulator, QWidget *parent = 0);

        /**
         Calculates the preferred minimum size.
         @return The preferred minimum size.
         */
        QSize minimumSizeHint() const;

        /**
         Calculates the preferred size.
         @return The preferred size.
         */
        QSize sizeHint() const;
};

#endif

#endif // WITH_GUI
