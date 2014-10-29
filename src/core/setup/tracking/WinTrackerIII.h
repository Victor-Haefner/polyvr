/*
** WTracker.h
** 
** Made by Mathias Broxvall
** Login   <mbl@localhost>
** 
** Started on  Mon Mar 17 19:07:14 2008 Mathias Broxvall
** Last update Mon Mar 17 19:23:22 2008 Mathias Broxvall
*/

#ifndef   	WTRACKER_H_
# define   	WTRACKER_H_

int initialize_wtracker();
void shutdown_wtracker();
void tick_wtracker();
void enable_cont_mode();
void disable_cont_mode();

#define N_SENSORS 3

typedef struct WTrackerSensor {
  double x,y,z;
  double a,e,r;
  double qw,qx,qy,qz;
  short int button;
} WTrackerSensor;

extern WTrackerSensor wtrackerSensors[N_SENSORS];

void setUpHemisphere();
void setFrontHemisphere();

#endif 	    /* !WTRACKER_H_ */
