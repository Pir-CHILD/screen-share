# screen-share
screen sharing for developers

使用C++实现[这个](https://github.com/screego/server)项目

## Architecture: 
``````
 _________               ___________               ________  
|         |   1.push    |           |  3.create   |        |
| sharing | ----------> | ws_server | ----------> | worker |
|_________|             |___________|             |________|
                              ^                       ^
                              |                       |
                              | 2.request             | 4.connect
                              |                       |
                              |                       |
                          _________                   |
                         |         |                  |
                         | shared  | <----------------+
                         |_________|   
