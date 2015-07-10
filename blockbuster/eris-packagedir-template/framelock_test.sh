#!/bin/bash
#
#  Dale's magic script to enable stereo framelock
#  dsouth@llnl.gov
#

MYHOST=`/bin/uname -n`
case $MYHOST in
  stagg0)
    MSTR=stagg1
    SLVS=(stagg2 stagg3 stagg4)
    ;;
  boole0)
    MSTR=boole1
    SLVS=(boole2 boole3 boole4)
    ;;
  grant0)
    MSTR=grant1
    SLVS=(grant2 grant3)
    ;;
  *)
    echo "  host ${MYHOST} isn't supported by this script"
    exit
esac

# setup X for QBS
for i in ${MSTR} ${SLVS[*]}
do
  /usr/nvidia/bin/nvidia-settings -n -a ${i}:0/AllowFlipping=1
  /usr/nvidia/bin/nvidia-settings -n -a ${i}:0/ForceStereoFlipping=0
done

# Disable FL so we can modifiy GPU configs
for i in ${MSTR} ${SLVS[*]}
do
  /usr/nvidia/bin/nvidia-settings -n -a ${i}:0[gpu:0]/FrameLockEnable=0
done

# Setup master GPU
FrameLockUseHouseSyncFlag=0
/usr/nvidia/bin/nvidia-settings -n -a ${MSTR}:0[gpu:0]/FrameLockMaster=1
/usr/nvidia/bin/nvidia-settings -n -a ${MSTR}:0[gpu:0]/FrameLockSlaves=2
/usr/nvidia/bin/nvidia-settings -n -a \
                       ${MSTR}:0[framelock:0]/FramelockUseHouseSync=${FrameLockUseHouseSyncFlag}
#                       ${MSTR}:0[framelock:0]/FramelockUseHouseSync=0

# Setup Slave GPUs
for i in ${SLVS[*]}
do
  /usr/nvidia/bin/nvidia-settings -n -a ${i}:0[gpu:0]/FrameLockMaster=0
  /usr/nvidia/bin/nvidia-settings -n -a ${i}:0[gpu:0]/FrameLockSlaves=3
done

# Enable FrameLock
for i in ${MSTR} ${SLVS[*]}
do
  /usr/nvidia/bin/nvidia-settings -n -a ${i}:0[gpu:0]/FrameLockEnable=1
done

# Remind user how to run BlockBuster
if [[ $? == 0 ]]
then
  echo " Stereo FrameLock is enabled."
  echo "FrameLockUseHouseSync = ${FrameLockUseHouseSyncFlag}"
  echo " Run blockbuster with  -r dmx -R gl_stereo  to use"
fi
