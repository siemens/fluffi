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

$(document).ready(function(){
    var data = { infoType: "timeOfLatestPopulation"} ;

    if (projId != undefined) {
        setInterval(function() {            
            $.ajax({
                url: "/projects/" + projId + "/updateInfo",
                type: 'POST',
                data: JSON.stringify(data),
                contentType: 'application/json; charset=utf-8',
                dataType: 'json',
                success: function(response) {
                    if(response["status"] == "OK"){
                        $("#timeOfLatestPopulation").css("background-color", "#1abc9c");
                        $("#timeOfLatestPopulation").text(response["info"]);
                        setTimeout(function(){ $("#timeOfLatestPopulation").css("background-color", "#FFFFFF"); }, 500);
                    } else {
                        console.log(response["message"]);
                    }               
                }
            });
        }, 60000);       
    } else {
        console.log("Project ID should not be undefined!");
    }       
});
