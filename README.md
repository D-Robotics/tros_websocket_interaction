
# 下载代码

# 编译和安装

```bash
mkdir build; cd build
cmake ..
make -j6 && make install && cd -
```

# 运行

```bash
source /opt/tros/humble/setup.bash
cp -r /opt/tros/${TROS_DISTRO}/lib/hobot_yolo_world/config/ .
ros2 launch install/launch/yolo_world.launch.py yolo_world_texts:="red bottle,trash bin" smart_topic:=/hobot_yolo_world

./install/tros_websocket_client http://192.168.3.123:8081 install/config/yolo_world_test.jpg

```