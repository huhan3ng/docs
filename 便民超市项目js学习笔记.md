1.

页面渲染https://coolshell.cn/articles/9666.html

浏览器提供js接口用以操作html dom节点。

jquery也封装了更便捷的dom选择器。

http://www.w3school.com.cn/jquery/jquery_selectors.asp



2.

下拉列表的实现：注册onclick,ontouch等回掉，通过手指滑动距离相应增加head div长度，并且dy随着y增大而减小，从而使页面看起来像是果冻有弹性，在被拖动。



3.

柱状图，饼状图等实现，类似android，浏览器提供canvas画布，确定坐标系，根据图形函数绘制即可。动画可以由定时调用invalidate时canvas重绘实现。



4.

网络处理多用 ajax, 异步回掉。类似android上的retrofit2，属于观察者模式的应用，微信小程序也是。