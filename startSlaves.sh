#!/bin/sh

ssh viscluster51 'export DISPLAY=":0.0" && /mnt/raid/home/hpcwoess/PolyVR/polyvrslave/src/cluster/start -m s51 > /dev/null 2> /dev/null < /dev/null &' &
ssh viscluster52 'export DISPLAY=":0.0" && /mnt/raid/home/hpcwoess/PolyVR/polyvrslave/src/cluster/start -m s52 > /dev/null 2> /dev/null < /dev/null &' &
ssh viscluster53 'export DISPLAY=":0.0" && /mnt/raid/home/hpcwoess/PolyVR/polyvrslave/src/cluster/start -m s53 > /dev/null 2> /dev/null < /dev/null &' &
ssh viscluster54 'export DISPLAY=":0.0" && /mnt/raid/home/hpcwoess/PolyVR/polyvrslave/src/cluster/start -m s54 > /dev/null 2> /dev/null < /dev/null &' &
ssh viscluster55 'export DISPLAY=":0.0" && /mnt/raid/home/hpcwoess/PolyVR/polyvrslave/src/cluster/start -m s55 > /dev/null 2> /dev/null < /dev/null &' &
ssh viscluster56 'export DISPLAY=":0.0" && /mnt/raid/home/hpcwoess/PolyVR/polyvrslave/src/cluster/start -m s56 > /dev/null 2> /dev/null < /dev/null &' &
ssh viscluster57 'export DISPLAY=":0.0" && /mnt/raid/home/hpcwoess/PolyVR/polyvrslave/src/cluster/start -m s57 > /dev/null 2> /dev/null < /dev/null &' &
ssh viscluster58 'export DISPLAY=":0.0" && /mnt/raid/home/hpcwoess/PolyVR/polyvrslave/src/cluster/start -m s58 > /dev/null 2> /dev/null < /dev/null &' &
ssh viscluster59 'export DISPLAY=":0.0" && /mnt/raid/home/hpcwoess/PolyVR/polyvrslave/src/cluster/start -m s59 > /dev/null 2> /dev/null < /dev/null &' &
ssh viscluster60 'export DISPLAY=":0.0" && /mnt/raid/home/hpcwoess/PolyVR/polyvrslave/src/cluster/start -m s60 > /dev/null 2> /dev/null < /dev/null &' &

