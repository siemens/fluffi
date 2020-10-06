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

$(document).ready(function() {

    var canvas = document.getElementById('canvas');
    var context = canvas.getContext('2d');    

    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;

    var projId = window.location.hash.substr(1);
    var url = projId === "all" ? "/projects/coverageDistribution" : "/projects/" + projId + "/coverageDistribution";

    $.getJSON(url, function(response) {                 
        drawCircles(response["data"]);
    });     

    function getRandomColor() {
        var letters = '0123456789ABCDEF';
        var color = '#';
        for (var i = 0; i < 6; i++) {
            color += letters[Math.floor(Math.random() * 16)];
        }

        return color == "#FFFFFF" ? "#011638" : color;
    }    

    function drawCircles(circles) {
        var pi2 = Math.PI * 2;

        var cxFirst = canvas.width / 4;
        var cx = cxFirst;
        var cy = canvas.height / 4;

        var firstRadius = circles.length > 0 ? circles[0].radius : 0;
        var oldRadius, newRadius;

        for (var index = 0; index < circles.length; index++) {
            oldRadius = circles[index].radius;
            context.beginPath();
            context.moveTo(cx, cy);
            context.arc(cx, cy, oldRadius, 0, pi2, false);
            context.fillStyle = getRandomColor();                 
            context.fill(); 

            context.textAlign = 'center';
            context.fillStyle = 'white'; 
            context.fillText(circles[index].title, cx, cy); 
            
            if(index < circles.length - 1){
                newRadius = circles[index + 1].radius
            }
            
            if(index > 0 && index % 5 == 0){
                cx = cxFirst;
                cy = cy + firstRadius + newRadius;
            } else {
                cx = cx + (oldRadius + newRadius);
            }                                              
        }        
    }
});
