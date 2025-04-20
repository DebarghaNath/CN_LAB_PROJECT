# CN_LAB_PROJECT

#Installation Step:
https://chatgpt.com/share/67f38fd2-72a0-800d-b4d7-6c455cebfbb4


find build/scratch/ -type f -perm -111 -exec file {} \; | grep "executable"
./build/scratch/ns3-dev-first-sim-default


cmake --build . --target scratch_first-sim
/Users/debar/ns-3-allinone/ns-3-dev/build/scratch/ns3-dev-first-sim-default
cd ~/ns-3-allinone/ns-3-dev/cmake-cache

./ns3 run scratch/project
