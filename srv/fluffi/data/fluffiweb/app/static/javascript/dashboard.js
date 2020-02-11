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
  if(myProjectName == 'grafana1'){      
    return "http://mon.fluffi/d/MAzsmLaik/fluffi-1?orgId=1&refresh=5s&kiosk";
  } 
  if(myProjectName == 'grafana2'){      
    return "http://mon.fluffi/d/jlWq9GtWk/fluffi-2?orgId=1&refresh=5s&kiosk";
  }    
  
  return "http://mon.fluffi/dashboard/script/scripted.js?fuzzjobname=" + myProjectName + "&from=" + from + "&to=" + to + "&orgId=1&kiosk";
}
