/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Junes Najah, Thomas Riedmaier, Michael Kraus
*/

//needs to be adjusted if columns are added or removed
var TIME_RANGE_CELL_INDEX = 6;
var states = ['now-15m', 'now-24h', 'now-7d'];
var timeRanges = {'now-15m': 'Last 15 min', 'now-24h': 'Last Day', 'now-7d': 'Last Week'};
var LOADING_TIME = 50000; 
var SHOW_TIME = 10000 + LOADING_TIME; 
var INTERVAL_TIME = SHOW_TIME * states.length;
var NOW = 'now'; 

/*** global variables ***/
var reloadCounter = 1;
var rowIndex = 1;
var socket, table, tableSize, previousRow, row, projectName;

  $(function () { 
    socket = io.connect('http://dashsync.fluffi:4000'  );
    // verify our websocket connection is established
    socket.on('connect', function() {
        console.log('Websocket connected!');
    });

    //run after SHOW_TIME to keep time for grafana 
    setTimeout(function(){
      Run()
    }, SHOW_TIME);    
  });

  function Run(){    
    //reload every minute until projects exist
    tableSize = $('#fuzzjobTable tbody tr').length;
    if(tableSize == 0){
      //still show grafana monitoring if there are no active fuzzjobs
      //load and show grafana 1
      console.log("send load grafana 1");
      socket.emit('syncProject', {send: 'load', project: 'grafana1', time: 'noTime', time2: 'noTime'});   
      setTimeout(function(){    
        console.log("send show grafana 1");
        setRowInactive();
        socket.emit('syncProject', {send: 'show', project: 'grafana1', time: 'noTime', time2: 'noTime'});
      }, LOADING_TIME);   

      setTimeout(function(){
        //load and show grafana 2
        console.log("send load grafana 2");
        socket.emit('syncProject', {send: 'load', project: 'grafana2', time: 'noTime', time2: 'noTime'});   
        setTimeout(function(){    
          console.log("send show grafana 2");
          socket.emit('syncProject', {send: 'show', project: 'grafana2', time: 'noTime', time2: 'noTime'});
          location.reload(true);
        }, LOADING_TIME); 
      }, SHOW_TIME);       
    } else {
      table = document.getElementById("fuzzjobTable");
      //first init 
      loadAndShowProj();

      setInterval(function(){                    
        loadAndShowProj();
        reloadCounter++;
      }, INTERVAL_TIME);
    }
  }
  
  function loadAndShowProj(){
    //if tableSize is 1 we need to add one interval to show one project and then switch to grafana
    tableSize = tableSize == 1 ? tableSize+1 : tableSize;
    console.log("reloadCounter", reloadCounter);
    console.log("tableSize", tableSize);
    if(reloadCounter == tableSize){
      //load and show grafana 1
      console.log("send load grafana 1");
      socket.emit('syncProject', {send: 'load', project: 'grafana1', time: 'noTime', time2: 'noTime'});   
      setTimeout(function(){    
        console.log("send show grafana 1");
        setRowInactive();
        socket.emit('syncProject', {send: 'show', project: 'grafana1', time: 'noTime', time2: 'noTime'});
      }, LOADING_TIME);   

      setTimeout(function(){
        //load and show grafana 2
        console.log("send load grafana 2");
        socket.emit('syncProject', {send: 'load', project: 'grafana2', time: 'noTime', time2: 'noTime'});   
        setTimeout(function(){    
          console.log("send show grafana 2");
          socket.emit('syncProject', {send: 'show', project: 'grafana2', time: 'noTime', time2: 'noTime'});
          location.reload(true);
        }, LOADING_TIME); 
      }, SHOW_TIME);      
    } else {
      rowIndex = rowIndex > (table.rows.length-1) ? 1 : rowIndex;             
      previousRow = getPreviousRow();
      row = table.rows[rowIndex];          
      projectName = row.cells[0].firstChild.innerHTML;
      
      if(row && projectName){
        emitStatesForProj(projectName);
      } else {
        console.log("Error while getting the project name!");
      }        
      rowIndex++;
    }          
  }

  function getPreviousRow(){
    if(rowIndex == 1)
      return table.rows[table.rows.length-1];
    else if(rowIndex > 1)
      return table.rows[rowIndex-1];
    else
      return null;
  }

  function setRowActive(){
    // first set previous row inactive if it exists
    if(previousRow){
      previousRow.cells[TIME_RANGE_CELL_INDEX].innerHTML="";
      previousRow.style.backgroundColor = "#222";
    }
    //change current row color to active
    row.style.backgroundColor = "#3498DB";
  }

  function setRowInactive(){
    row.cells[TIME_RANGE_CELL_INDEX].innerHTML="";
    row.style.backgroundColor = "#222";
  }
 
  function emitStatesForProj(myProjName){            
    //init
    console.log("send load", myProjName, states[0]); 
    socket.emit('syncProject', {send: 'load', project: myProjName, time: states[0], time2: NOW});

    setTimeout(function(){    
      console.log("send show", myProjName, states[0]);  
      socket.emit('syncProject', {send: 'show', project: myProjName, time: states[0], time2: NOW});
      setRowActive();
      changeTimeRange(timeRanges[states[0]]);  
    }, LOADING_TIME);

    states.forEach(function(state, i){
      if(i != 0){
        setTimeout(function(){
          console.log("send load", myProjName, state); 
          socket.emit('syncProject', {send: 'load', project: myProjName, time: state, time2: NOW}); 
          setTimeout(function(){
            console.log("send show", myProjName, state); 
            socket.emit('syncProject', {send: 'show', project: myProjName, time: state, time2: NOW}); 
            changeTimeRange(timeRanges[state]);
          }, LOADING_TIME);
        }, SHOW_TIME*i);
      }      
    });
  }

  function changeTimeRange(timeRange){
    row.cells[TIME_RANGE_CELL_INDEX].innerHTML="";
    row.cells[TIME_RANGE_CELL_INDEX].innerHTML=timeRange;
  }
