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

Author(s): Junes Najah, Thomas Riedmaier, Pascal Eckmann
*/

var elemType, myGUID, myLocalID, myCommand;

function addOrRenameNiceName(index, projectId, command, tcGUID, tcLocID, miGUID){
    var url = "/projects/" + projectId + "/renameElement";
    myCommand = command;

    if(tcGUID){
        myGUID = tcGUID;
        myLocalID = tcLocID;
        elemType = "testcase";
    }        
    else if(miGUID){
        myGUID = miGUID;
        myLocalID = -1;
        elemType = "managedInstance";
    } else{
        console.log("Error: Invalid Id argument in addOrRenameNiceName(...)!");
    }      

    if(command == "update"){        
        var btnId = "#tcBtn";
        var inputId = "#tcInput";
        var inputIdHtml = "tcInput";
    }        
    else if(command == "insert") {        
        var btnId = "#tcBtnNew";
        var inputId = "#tcInputNew";
        var inputIdHtml = "tcInputNew";
    } else {
        console.log("Error: Invalid command argument in addOrRenameNiceName(...)!")
    }        

    $(btnId+index).css("display", "none");
    $(inputId+index).css("display", "inline");
    $(inputId+index).focus();

    $(inputId+index).focusout(function() {
        $(inputId+index).css("display", "none");
        $(btnId+index).css("display", "inline");        
    });

    var inputField = document.getElementById(inputIdHtml+index);
    inputField.addEventListener("keydown", function (e) {
        if (e.keyCode === 13) {  //checks whether the pressed key is "Enter"
            validateAndSave(e, index, url, btnId, inputId);
        }
    });
}

function validateAndSave(e, i, url, btnId, inputId) {
    var input = e.target.value;
    var data = { "newName": input, "elemType": elemType, "myGUID": myGUID, "myLocalID": myLocalID, "command": myCommand};

    if(input.length != 0){
        $(inputId+i).css("display", "none");
        $(btnId+i).css("display", "inline");  
        
        $(btnId+i).text(input);

        //update in DB and view if successful
        $.ajax({
        url: url,
        type: 'POST',
        data: JSON.stringify(data),
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: true,
        success: function(response) {                            
                if(response["status"] == "OK"){
                    console.log("Success: " + response["message"]);
                    if(response["command"] == "insert"){
                        location.reload();
                    }
                }                    
                else {
                    alert(response["message"]);                    
                }                    
            }
        });
    } else {
        $(inputId+i).css("display", "none");
        $(btnId+i).css("display", "inline");   
    }           
}
