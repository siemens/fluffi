/*
Copyright 2017-2020 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including without
limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

Author(s): Junes Najah
*/

var MON_BASE_URL = "http://mon.fluffi";
var MON_PORT = ":8885";

var socket;
var id = "#icontainer";
var i = 0;

var send = "";
var myProjectID = "";
var myTime = "";
var myTime2 = "";

$(function () {
  var socket = io.connect('http://dashsync.fluffi:4000' );    
  // verify our websocket connection is established
  socket.on('connect', function() {
      console.log('Websocket connected!');
  });

  socket.on('syncevent', function(data) {
      
      console.log("websocket data: ", data['send'], data['project'], data['time'], data["time2"]); 
      send = data['send'];
      myProjectID = data["project"];
      myTime = data["time"];
      myTime2 = data["time2"];

      if(send && myProjectID && myTime && myTime2){
        if(send == 'load'){
        //switch i to next icontainer
        i = i == 0 ? ++i : --i;

        //load next icontainer
        console.log("loading icontainer" + i);
        $(id + i).empty();
        $(id + i).append("<iframe width='100%' src='" + getURL(myProjectID, myTime, myTime2) + "' allowfullscreen/>");   

        //switch i back to current icontainer        
        i = i == 0 ? ++i : --i;    
        }
        else if(send == 'show'){    
          //hide current icontainer      
          $(id + i).css('visibility','hidden');    
          console.log("hide icontainer" + i);
          //switch i to next icontainer and make next icontainer visible
          i = i == 0 ? ++i : --i; 
          
          $(id + i).css('visibility','visible');    
          console.log("show icontainer" + i);
      } 

    } else { console.log("Data of websocket is empty or undefined..."); }           
  });

});

function getURL(myProjectName, from, to){
  // TODO is port always needed ?!
  if(myProjectName == 'grafana1'){      
    return MON_BASE_URL + MON_PORT + "/d/MAzsmLaik/fluffi-1?orgId=1&refresh=5s&kiosk";
  } 
  if(myProjectName == 'grafana2'){      
    return MON_BASE_URL + MON_PORT + "/d/jlWq9GtWk/fluffi-2?orgId=1&refresh=5s&kiosk";
  }    
  
  return MON_BASE_URL + MON_PORT + "/dashboard/script/scripted.js?fuzzjobname=" + myProjectName + "&from=" + from + "&to=" + to + "&orgId=1&kiosk";
}
