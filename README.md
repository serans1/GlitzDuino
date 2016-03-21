# GlitzDuino
This is an arduino based  [Glitz](http://www.theglitzproject.com) device, used for prototyping and App development testing. 

This code allows you to create a Glitz device using Arduino and Bluetooth serial module (HC-06).

##commands 
This device is not BLE (4.0) like the production one but exposes command/data interface through bluetooth SPP, example on SPP with xamarin - http://brianpeek.com/post/Connect-to-a-Bluetooth-Device-with-XamarinAndroid

The device has pass word of 1234 and gets command in the format "(address,data"), the addresses are as follows:
5 - set the like levels => 0 - 100, where 0 is blue and 100 is red.  
6 - set the heartbit on time in ms, value is 100ms units. 
7 - set the heartbit off time in ms, value is 100ms units. 
8 - set the heartbit dimming time in ms, value is 100ms units. 
9- set the flicker on time in ms, value is 10ms units. 
10 - set the flicker off time in ms, value is 10ms units. 
11 - set the flicker count per like level update 
##pictures

![alt text](https://github.com/serans1/GlitzDuino/blob/master/20160321_221531.jpg "Glitz arduino device")

