#ifdef WITH_GUI

#include <QtGui>

#include "renderarea.h"
#include "window.h"

#include "../Routing.h"
#include "../TrafficSimulator.h"
#include "../RoadSystem.h"
#include "../NodeLogicRightFirst.h"
#include "../NodeLogicTrafficLight.h"

RenderArea::RenderArea(TrafficSimulator *simulator, QWidget *parent)
  : QWidget(parent), simulator(simulator), roadSystem(simulator->getRoadSystem()) , zoom(8), position(-width() / 2 - 600, height() / 2 + 500),
    nearestVehicle(0), nearestNode(NULL), nearestStreet(NULL), nearestNodeLogic(NULL), lastPauseButtonChecked(false) {

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(100);

    lastShow = "";
}

QSize RenderArea::minimumSizeHint() const {
    return QSize(100, 100);
}

QSize RenderArea::sizeHint() const {
    return QSize(400, 200);
}

QPointF RenderArea::transform(QPointF p) {
    QPointF newP(p * zoom + position);
    int tmp = newP.x();
    newP.setX(newP.y());
    newP.setY(-tmp);
    return newP;
}

double RenderArea::scale(double n) {
    return n * zoom;
}

void RenderArea::drawLine(QPainter& painter, Vec2f start, Vec2f end, bool capMaxWidth) {
    drawLine(painter, QPointF(start[0], start[1]), QPointF(end[0], end[1]), capMaxWidth);
}

void RenderArea::drawLine(QPainter& painter, QPointF start, QPointF end, bool capMaxWidth) {
    QPen oldPen = painter.pen();
    QPen newPen = painter.pen();
    double myWidth = std::max(scale(oldPen.widthF()), 1.0);
    if (capMaxWidth)
        myWidth = std::min(myWidth, oldPen.widthF());
    newPen.setWidth(myWidth);
    painter.setPen(newPen);
    painter.drawLine(transform(start), transform(end));
    painter.setPen(oldPen);
}

void RenderArea::drawEllipse(QPainter& painter, Vec2f pos, double radius) {
    drawEllipse(painter, QPointF(pos[0], pos[1]), radius);
}

void RenderArea::drawEllipse(QPainter& painter, QPointF pos, double radius) {
    painter.drawEllipse(transform(pos), (int)scale(radius), (int)scale(radius));
}

void RenderArea::drawRect(QPainter& painter, Vec2f pos, double size) {
    drawRect(painter, QPointF(pos[0], pos[1]), size);
}

void RenderArea::drawRect(QPainter& painter, QPointF pos, double size) {
    painter.drawRect(transform(pos).x(), transform(pos).y(), scale(size), scale(size));
}

void RenderArea::drawText(QPainter& painter, Vec2f pos, string text) {
    drawText(painter, QPointF(pos[0], pos[1]), tr(text.c_str()));
}

void RenderArea::drawText(QPainter& painter, QPointF pos, QString text) {
    QFont oldPen = painter.font();
    QFont newPen = painter.font();
    double myWidth = std::min(std::max(scale(oldPen.pointSizeF()), 1.0), oldPen.pointSizeF());
    newPen.setPointSizeF(myWidth);
    painter.setFont(newPen);
    painter.drawText(transform(pos), text);
    painter.setFont(oldPen);
}

void RenderArea::drawNode(QPainter& painter, Node *node) {


    if (node->getFeatures() & Node::TRAFFICLIGHTS)
        painter.setBrush(Qt::SolidPattern);
    else
        painter.setBrush(Qt::NoBrush);
    QPen pen = painter.pen();
    pen.setStyle(Qt::DashLine);
    painter.setPen(pen);
    drawEllipse(painter, node->getPosition(), 2);

    if (node->getNodeLogic() != NULL && node->getNodeLogic()->getType() == NodeLogic::TRAFFIC_LIGHT) {

        NodeLogicTrafficLight *logic = static_cast<NodeLogicTrafficLight*>(node->getNodeLogic());
        const vector<NodeLogicTrafficLight::LightPost>& lights = logic->getLightPostState(node->getId());

        if (!lights.empty()) {

            // Hm... Can it be empty? I think it shouldn't, but it does. :-/
            // On the other hand: Who cares whether it can happen? It works, that is all I need to know

            // Draw the lights
            for (size_t lightI = 0; lightI < lights.size(); ++lightI) {

                Vec2f facingPos = roadSystem->getNode(lights[lightI].facing)->getPosition();
                // Calculate an offset of some pixel
                Vec2f offset = (facingPos - node->getPosition());
                offset.normalize();
                offset *= 5;

                Street *street = roadSystem->getStreet(lights[lightI].street);

                // Draw circles for all lanes
                for (unsigned int k = 1; k <= lights[lightI].laneStates.size(); ++k) {

                    Vec2f lightPos = street->getRelativeNodePosition(node->getId(), lights[lightI].direction * k);
                    switch(lights[lightI].laneStates[k - 1]) {
                        case NodeLogicTrafficLight::GREEN:
                            painter.setBrush(Qt::green);
                            break;
                        case NodeLogicTrafficLight::AMBER:
                            painter.setBrush(Qt::yellow);
                            break;
                        case NodeLogicTrafficLight::RED:
                            painter.setBrush(Qt::red);
                            break;
                        case NodeLogicTrafficLight::RED_AMBER:
                            painter.setBrush(Qt::magenta);
                            break;
                        default:
                            painter.setBrush(Qt::gray);
                    }
                    drawEllipse(painter, lightPos + offset, 1);
                    string turns;
                    Street::LANEFLAG flags = street->getLaneFlags(lights[lightI].direction * k);
                    if (flags & Street::TURN_LEFT)
                        turns += "l";
                    if (flags & Street::TURN_THROUGH)
                        turns += "t";
                    if (flags & Street::TURN_RIGHT)
                        turns += "r";
                    painter.setPen(Qt::darkRed);
                    drawText(painter, lightPos + offset + Vec2f(-0.5,-0.5), turns);
                    painter.setPen(Qt::black);
                }
            }
        }
    }
    painter.setBrush(Qt::NoBrush);
}

void RenderArea::drawStreet(QPainter& painter, Street *street) {

    const vector<ID>& nodes = *(street->getNodeIds());
    for (unsigned int i = 0; i < nodes.size() - 1; ++i) {

        for (int direction = -1; direction < 2; direction += 2) {

            char labelText[512] = "";

            if (!street->getIsMicro()) {
                // If it is a meso street, color it
                double percentage;
                Street::TYPE type = street->getType();
                switch ((static_cast<Window*>(parent())->mesoTypeComboBox->itemData(static_cast<Window*>(parent())->mesoTypeComboBox->currentIndex())).toInt()) {
                    case MaxSpeed:
                        if (direction > 0)
                            sprintf(labelText, "%.1f", street->getMaxSpeed());
                        percentage = ((double)street->getMaxSpeed()/150);
                        painter.setPen(QPen(QColor(255 * percentage, 255 * (1-percentage), 0)));
                        break;
                    case RelativeVehicleCount:
                        sprintf(labelText, "%u/%d", street->getLaneVehicleCount(direction), street->getLaneMaxVehicleCount(direction));
                        percentage = ((double)street->getLaneVehicleCount(direction)/street->getLaneMaxVehicleCount(direction));
                        if (percentage > 1)
                            percentage = 1;
                        painter.setPen(QPen(QColor(255 * percentage, 255 * (1-percentage), 0)));
                        break;
                    case Type:
                        if (direction > 0) {
                            switch(type) {
                                case Street::MOTORWAY: sprintf(labelText, "MOTORWAY"); break;
                                case Street::TRUNK: sprintf(labelText, "TRUNK"); break;
                                case Street::PRIMARY: sprintf(labelText, "PRIMARY"); break;
                                case Street::SECONDARY: sprintf(labelText, "SECONDARY"); break;
                                case Street::TERTIARY: sprintf(labelText, "TERTIARY"); break;
                                case Street::RESIDENTIAL: sprintf(labelText, "RESIDENTIAL"); break;
                                default: sprintf(labelText, "OTHER"); break;
                            }
                        }

                        if (type == Street::MOTORWAY || type == Street::TRUNK || type == Street::PRIMARY)
                            painter.setPen(QPen(QColor(30, 144, 255)));
                        else if (type == Street::SECONDARY || type == Street::TERTIARY)
                            painter.setPen(QPen(QColor(255, 165, 0)));
                        else
                            painter.setPen(Qt::gray);
                        break;
                    case Direction:
                        if (direction > 0)
                            painter.setPen(Qt::green);
                        else
                            painter.setPen(Qt::red);
                        break;
                    default:
                        painter.setPen(Qt::gray);
                }
            } else {
                painter.setPen(Qt::gray);
            }

            QPen p = painter.pen();
            p.setWidthF(roadSystem->getLaneWidth());
            painter.setPen(p);
            for (unsigned int lane = 1; lane <= street->getLaneCount(direction); ++lane) {
                Vec2f start = street->getRelativeNodePosition(nodes[i], direction * lane);
                Vec2f end = street->getRelativeNodePosition(nodes[i + 1], direction * lane);
                drawLine(painter, start, end, false);
            }


            if (i == (nodes.size() - 1) / 2 && static_cast<Window*>(parent())->mesoLabelCheckBox->isChecked()) {
                if (direction > 0)
                    painter.setPen(Qt::black);
                else
                    painter.setPen(Qt::darkGray);
                Vec2f start = street->getRelativeNodePosition(nodes[i], direction * (street->getLaneCount(direction) + 1));
                Vec2f end = street->getRelativeNodePosition(nodes[i + 1], direction * (street->getLaneCount(direction) + 1));
                drawText(painter, (start + end) / 2, labelText);
            }
        }
    }
}

void RenderArea::drawVehicle(QPainter& painter, Vehicle *vehicle) {

    // Do not draw vehicles in tunnel
    if (vehicle->getStreetId() == 4303949 && (nearestStreet == NULL || nearestStreet->getId() != 4303949))
        return;
    if (vehicle->getStreetId() == 4304013 && (nearestStreet == NULL || nearestStreet->getId() != 4304013))
        return;

    double radius = roadSystem->getVehicleType(vehicle->getVehicleType())->getRadius();
    drawEllipse(painter, Vec2f(vehicle->getPosition()[0], vehicle->getPosition()[2]), radius);
    Vec3f direction;
    vehicle->getOrientation().multVec(Vec3f(radius, 0, 0), direction);
    drawLine(painter, toVec2f(vehicle->getPosition()), toVec2f(vehicle->getPosition() + direction));
}

void RenderArea::paintEvent(QPaintEvent*) {

    QPainter painter(this);

    // Start/Pause the simulator
    // The lastPauseButtonChecked attribute is needed to avoid overwriting the state send over the network
    const bool buttonChecked = static_cast<Window*>(parent())->startStopButton->isChecked();
    const bool simulatorRunning = simulator->getState() == TrafficSimulator::RUN;
    const bool simulatorPaused  = simulator->getState() == TrafficSimulator::PAUSE;

    if (buttonChecked && !lastPauseButtonChecked && !simulatorRunning) {
        // Button has been toggled on
        simulator->setState(TrafficSimulator::RUN);
        lastPauseButtonChecked = buttonChecked;

    } else if (!buttonChecked && !lastPauseButtonChecked && simulatorRunning) {
        // Started per network
        static_cast<Window*>(parent())->startStopButton->setChecked(true);
        lastPauseButtonChecked = true;

    } else if (!buttonChecked && lastPauseButtonChecked && simulatorRunning) {
        // Button has been toggled off
        simulator->setState(TrafficSimulator::PAUSE);
        lastPauseButtonChecked = buttonChecked;

    } else if (buttonChecked && lastPauseButtonChecked && simulatorPaused) {
        // Paused per network
        static_cast<Window*>(parent())->startStopButton->setChecked(false);
        lastPauseButtonChecked = false;
    }

    // Single step
    if (static_cast<Window*>(parent())->singleStepCheckBox->isChecked() && simulatorRunning) {
        // Make button unchecked, will pause next tick
        static_cast<Window*>(parent())->startStopButton->setChecked(false);
    }

    // Draw everything

    painter.setRenderHint(QPainter::Antialiasing, true);

    // Updates the entries in the side row
    static char buf[32];
    sprintf(buf, "#Nodes:\t\t%lu", roadSystem->getNodes()->size());
    static_cast<Window*>(parent())->nodeCountLabel->setText(tr(buf));
    sprintf(buf, "#Streets:\t\t%lu", roadSystem->getStreets()->size());
    static_cast<Window*>(parent())->streetCountLabel->setText(tr(buf));
    sprintf(buf, "#Micro-Vehicles:\t%lu", roadSystem->getVehicles()->size());
    static_cast<Window*>(parent())->microVehicleCountLabel->setText(tr(buf));
    sprintf(buf, "#Offmap-Vehicles:\t%d", roadSystem->getOffmapVehicleCount());
    static_cast<Window*>(parent())->offmapVehicleCountLabel->setText(tr(buf));

    size_t count = 0;
    // Iterate over all streets and count their meso-vehicles:
    for (map<ID, Street*>::const_iterator iter = roadSystem->getStreets()->begin(); iter != roadSystem->getStreets()->end(); ++iter) {
        if (iter->second->getIsMicro())
            continue;
        count += iter->second->getLaneVehicleCount(-1);
        count += iter->second->getLaneVehicleCount(1);
    }
    sprintf(buf, "#Meso-Vehicles:\t%lu", count);
    static_cast<Window*>(parent())->mesoVehicleCountLabel->setText(tr(buf));

    sprintf(buf, "Sum Vehicles:\t\t%lu", roadSystem->getVehicles()->size() + count + roadSystem->getOffmapVehicleCount());
    static_cast<Window*>(parent())->sumVehicleCountLabel->setText(tr(buf));



    painter.setBrush(Qt::NoBrush);

    // Color is set inside the method
    for (map<ID, Street*>::const_iterator iter = roadSystem->getStreets()->begin();
        iter != roadSystem->getStreets()->end(); ++iter) {
        drawStreet(painter, iter->second);
    }

    painter.setPen(Qt::black);
    for (map<ID, Node*>::const_iterator iter = roadSystem->getNodes()->begin();
        iter != roadSystem->getNodes()->end(); ++iter) {
        drawNode(painter, iter->second);
    }

    painter.setPen(Qt::red);
    for (map<ID, Vehicle*>::const_iterator iter = roadSystem->getVehicles()->begin();
        iter != roadSystem->getVehicles()->end(); ++iter) {
        drawVehicle(painter, iter->second);
    }

    painter.setPen(Qt::blue);
    const RoadSystem::ViewArea *viewarea = roadSystem->getViewarea(0);
    if (viewarea != NULL)
        drawEllipse(painter, viewarea->position, viewarea->radius);



    for (set<NodeLogic*>::const_iterator iter = roadSystem->getNodeLogics()->begin();
        iter != roadSystem->getNodeLogics()->end(); ++iter) {
        // Connect center with nodes by lines

        if ((*iter)->getType() != NodeLogic::TRAFFIC_LIGHT)
            continue;

        painter.setPen(Qt::lightGray);
            drawEllipse(painter, (*iter)->getPosition(), ((NodeLogicTrafficLight*)(*iter))->getRadius());
        painter.setPen(Qt::magenta);
        for (set<ID>::iterator n = ((NodeLogicTrafficLight*)(*iter))->getTrafficLights().begin();
                n != ((NodeLogicTrafficLight*)(*iter))->getTrafficLights().end(); ++n) {
            drawLine(painter, (*iter)->getPosition(), roadSystem->getNode(*n)->getPosition());
        }
    }

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(palette().dark().color());
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRect(0, 0, width() - 1, height() - 1));

    // If the entry in the search box has changed, update the view and the label
    if (lastShow != static_cast<Window*>(parent())->nodeSearchBox->text()) {
        lastShow = static_cast<Window*>(parent())->nodeSearchBox->text();

        if (lastShow[0] == 'n') {
            // Center view on the position of the node
            // Do not change the zoom factor
            Node *node = roadSystem->getNode(QString(lastShow).remove(0, 1).toUInt());
            if (node != NULL) {
                nearestNode = node;
                Vec2f pos = node->getPosition();
                position = -QPoint(scale(pos[0]), scale(pos[1]));
                position.setX(position.x() - size().width() / 2);
                position.setY(position.y() + size().height() / 2);
            }
        } else if (lastShow[0] == 's') {
            // Center view on the position of the node
            // Do not change the zoom factor
            Street *street = roadSystem->getStreet(QString(lastShow).remove(0, 1).toUInt());
            if (street != NULL) {
                nearestStreet = street;
                Vec2f pos = roadSystem->getNode((*street->getNodeIds())[street->getNodeIds()->size() / 2])->getPosition();
                position = -QPoint(scale(pos[0]), scale(pos[1]));
                position.setX(position.x() - size().width() / 2);
                position.setY(position.y() + size().height() / 2);
            }
        } else if (lastShow[0] == 'v') {
            // Center view on the position of the node
            // Do not change the zoom factor
            Vehicle *vehicle = roadSystem->getVehicle(QString(lastShow).remove(0, 1).toUInt());
            if (vehicle != NULL) {
                nearestVehicle = vehicle->getId();
                Vec2f pos = toVec2f(vehicle->getPosition());
                position = -QPoint(scale(pos[0]), scale(pos[1]));
                position.setX(position.x() - size().width() / 2);
                position.setY(position.y() + size().height() / 2);
            }
        }
    }


    QPen pen;
    pen.setColor(Qt::cyan);
    pen.setWidth(3);
    painter.setPen(pen);
    if (nearestNodeLogic != NULL) {
        drawEllipse(painter, nearestNodeLogic->getPosition(), 5);
        if (nearestNodeLogic->getType() == NodeLogic::TRAFFIC_LIGHT) {
            for (set<ID>::iterator iter = static_cast<NodeLogicTrafficLight*>(nearestNodeLogic)->getTrafficLights().begin(); iter != static_cast<NodeLogicTrafficLight*>(nearestNodeLogic)->getTrafficLights().end(); ++iter)
                drawNode(painter, roadSystem->getNode(*iter));
            drawEllipse(painter, nearestNodeLogic->getPosition(), static_cast<NodeLogicTrafficLight*>(nearestNodeLogic)->getRadius());
        }
        static_cast<Window*>(parent())->dataLabel->setText(tr(nearestNodeLogic->toString().c_str()));
    } else if (nearestVehicle != 0) {
        Vehicle *vehicle = roadSystem->getVehicle(nearestVehicle);
        if (vehicle == 0) {
            nearestVehicle = 0;
        } else {

            // Braking distance
            double radius = pow(vehicle->getCurrentSpeed() / 10, 2);
            painter.setPen(Qt::darkRed);
            drawEllipse(painter, Vec2f(vehicle->getPosition()[0], vehicle->getPosition()[2]), radius);

            // Future position
            drawEllipse(painter, Vec2f(vehicle->getFuturePosition()[0], vehicle->getFuturePosition()[2]), 2);
            painter.setPen(pen);
            drawVehicle(painter, vehicle);
            static_cast<Window*>(parent())->dataLabel->setText(tr(vehicle->toString().c_str()));

            // Draw its route
            const deque<ID> *route = vehicle->getRoute();
            if (!route->empty()) {
                Vec2f lastPos;
                Vec2f pos = toVec2f(vehicle->getPosition());
                // From the vehicle to the sky
                for (size_t i = 1; i < route->size(); ++i) {
                    lastPos = pos;
                    pos = roadSystem->getNode(route->at(i))->getPosition();
                    drawLine(painter, lastPos, pos);
                }
                // From the first node (=last visited) to the vehicle
                painter.setPen(Qt::magenta);
                drawLine(painter, roadSystem->getNode(route->at(0))->getPosition(), toVec2f(vehicle->getPosition()));
                painter.setPen(pen);
                // From the vehicle to its current destination
                painter.setPen(Qt::green);
                drawLine(painter, toVec2f(vehicle->getPosition()), vehicle->getCurrentDestination());
                painter.setPen(pen);
            }

            // Near vehicles
            for (set<pair<double,ID> >::const_iterator vehicleIter = vehicle->nearVehicles.begin(); vehicleIter != vehicle->nearVehicles.end(); ++vehicleIter) {
                Vehicle *nearVehicle = roadSystem->getVehicle(vehicleIter->second);
                if (nearVehicle != NULL)
                    drawVehicle(painter, nearVehicle);
            }
        }
    }  else if (nearestNode != NULL) {
        drawNode(painter, nearestNode);
        static_cast<Window*>(parent())->dataLabel->setText(tr(nearestNode->toString().c_str()));
    } else if (nearestStreet != NULL) {
        const vector<ID>& nodes = *(nearestStreet->getNodeIds());
        for (unsigned int i = 0; i < nodes.size() - 1; ++i) {
            drawLine(painter, roadSystem->getNode(nodes[i])->getPosition(), roadSystem->getNode(nodes[i+1])->getPosition());
        }
        static_cast<Window*>(parent())->dataLabel->setText(tr(nearestStreet->toString().c_str()));
    }

    pen.setWidth(5);
    for (map<ID, Node*>::const_iterator iter = roadSystem->getNodes()->begin();
        iter != roadSystem->getNodes()->end(); ++iter) {

        if (! (iter->second->getFeatures() & Node::TRAFFICLIGHTS))
            continue;

        for (map<ID, Node*>::const_iterator iter2 = roadSystem->getNodes()->begin();
            iter2 != roadSystem->getNodes()->end(); ++iter2) {

            if (! (iter2->second->getFeatures() & Node::TRAFFICLIGHTS))
                continue;

            if (calcDistance(iter->second->getPosition(), iter2->second->getPosition()) > 100)
                continue;
        }
    }
}

void RenderArea::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        mouse_down = event->pos();
        mouse_last = event->pos();
    } else if (event->button() == Qt::MiddleButton) {
        zoom = 8;
        position = QPoint(-width() / 2, height() / 2);
    }
}

void RenderArea::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        QPoint delta = event->pos() - mouse_last;
        int tmp = delta.x();
        delta.setX(delta.y());
        delta.setY(-tmp);
        position -= delta;
        mouse_last = event->pos();
    } else if (event->buttons() & Qt::RightButton) {
        // Reverse the translations done while drawing
        QPointF pos(-event->posF().y(), event->posF().x());
        pos = (pos - position) / zoom;
        Vec2f posV(pos.x(), pos.y());
        Vehicle *vehicle = roadSystem->getVehicle(0);
        if (vehicle != NULL) {
            vehicle->setPosition(toVec3f(posV));
        }
    }
}

double dot(QPointF a, QPointF b) {
    return a.x() * b.x() + a.y() * b.y();
}

// Adapted from
// https://stackoverflow.com/questions/849211/shortest-distance-between-a-point-and-a-line-segment
double minimum_distance(QPointF v, QPointF w, QPointF p) {
    // Return minimum distance between line segment vw and point p
    const double l2 = QLineF(v, w).length();
    if (l2 == 0.0)
        return QLineF(p, v).length();   // v == w case
    // Consider the line extending the segment, parameterized as v + t (w - v).
    // We find projection of point p onto the line.
    // It falls where t = [(p-v) . (w-v)] / |w-v|^2
    const double t = dot(p - v, w - v) / (l2*l2);
    if (t < 0.0)
        return QLineF(p, v).length();       // Beyond the 'v' end of the segment
    else if (t > 1.0)
        return QLineF(p, w).length();  // Beyond the 'w' end of the segment
    const QPointF projection = v + t * (w - v);  // Projection falls on the segment
    return QLineF(p, projection).length();
}

void RenderArea::mouseReleaseEvent(QMouseEvent *event) {

    static char buf[32];

    if ((mouse_down - event->pos()).manhattanLength() < 5) {
        // It was a click, search what has been clicked.
        // Reverse the translations done while drawing
        QPointF pos(-event->posF().y(), event->posF().x());
        pos = (pos - position) / zoom;
        Vec2f posV(pos.x(), pos.y());

        // Search for an object at this position
        nearestVehicle = 0;
        nearestNode = NULL;
        nearestStreet = NULL;
        nearestNodeLogic = NULL;

        // Search for a vehicle
        double minDist = 2;
        for (map<ID, Vehicle*>::const_iterator vehicles = roadSystem->getVehicles()->begin(); vehicles != roadSystem->getVehicles()->end(); ++vehicles) {
            if (calcDistance(vehicles->second->getPosition(), posV) < roadSystem->getVehicleType(vehicles->second->getVehicleType())->getRadius()) {
                minDist = calcDistance(vehicles->second->getPosition(), posV);
                nearestVehicle = vehicles->first;
                sprintf(buf, "v%u", nearestVehicle);
                static_cast<Window*>(parent())->nodeSearchBox->setText(tr(buf));
                lastShow = tr(buf);
            }
        }
        if (nearestVehicle != 0) {
            // Print its route
            deque<ID> *route = roadSystem->getVehicle(nearestVehicle)->getRoute();
            cout << "Route of " << nearestVehicle << "\n";
            for (size_t i = 0; i < route->size(); ++i) {
               cout << route->at(i) << "\n";
            }
            return;
        }

        // Search for a node
        minDist = 2;
        for (map<ID, Node*>::const_iterator nodes = roadSystem->getNodes()->begin(); nodes != roadSystem->getNodes()->end(); ++nodes) {
            if (calcDistance(nodes->second->getPosition(), posV) < minDist) {
                minDist = calcDistance(nodes->second->getPosition(), posV);
                nearestNode = nodes->second;
                sprintf(buf, "n%u", nodes->first);
                static_cast<Window*>(parent())->nodeSearchBox->setText(tr(buf));
                lastShow = tr(buf);
            }
        }
        if (nearestNode != NULL)
            return;

        minDist = 5;
        for (set<NodeLogic*>::const_iterator nodeLogics = roadSystem->getNodeLogics()->begin(); nodeLogics != roadSystem->getNodeLogics()->end(); ++nodeLogics) {
            if (calcDistance((*nodeLogics)->getPosition(), posV) < minDist) {
                minDist = calcDistance((*nodeLogics)->getPosition(), posV);
                nearestNodeLogic = *nodeLogics;
                buf[0] = '\0';
                static_cast<Window*>(parent())->nodeSearchBox->setText(tr(buf));
                lastShow = tr(buf);
            }
        }
        if (nearestNodeLogic != NULL)
            return;


        // Iterate over all streets
        minDist = 10;
        for (map<ID, Street*>::const_iterator streets = roadSystem->getStreets()->begin(); streets != roadSystem->getStreets()->end(); ++streets) {
            // Iterate over all nodes of the street and calculate the distance for the part between them
            const vector<ID>& nodes = *(streets->second->getNodeIds());
            for (unsigned int i = 0; i < nodes.size() - 1; ++i) {
                QPointF a(roadSystem->getNode(nodes[i])->getPosition()[0], roadSystem->getNode(nodes[i])->getPosition()[1]);
                QPointF b(roadSystem->getNode(nodes[i+1])->getPosition()[0], roadSystem->getNode(nodes[i+1])->getPosition()[1]);
                if (minimum_distance(a, b, pos) < minDist) {
                    minDist = minimum_distance(a, b, pos);
                    nearestStreet = streets->second;
                    sprintf(buf, "s%u", streets->first);
                    static_cast<Window*>(parent())->nodeSearchBox->setText(tr(buf));
                    lastShow = tr(buf);
                }
            }
        }
    }
}

void RenderArea::wheelEvent(QWheelEvent *event) {

    // The camera always zooms to 0/0 of the map and is moving the
    // view while doing so :(

    if (event->delta() > 0)
        zoom += 0.2;
    else
        zoom -= 0.2;

    if (zoom > 20)
        zoom = 20;
    else if (zoom < 0.1)
        zoom = 0.1;

    event->accept();
}

#endif
