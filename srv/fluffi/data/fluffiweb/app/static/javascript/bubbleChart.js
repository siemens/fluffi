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
    var projId = window.location.hash.substr(1);

    // TODO: get data...
    // [{radius, title, color}, ...]

    var canvas = document.getElementById('canvas');
    var context = canvas.getContext('2d');    

    window.addEventListener('resize', resizeCanvas, false);

    function resizeCanvas() {
            canvas.width = window.innerWidth;
            canvas.height = window.innerHeight;
            
            drawCircles(50, 'target3'); 
    }
    resizeCanvas();

    function getRandomColor() {
        var letters = '0123456789ABCDEF';
        var color = '#';
        for (var i = 0; i < 6; i++) {
            color += letters[Math.floor(Math.random() * 16)];
        }
        return color;
    }

    function drawCircles(radius, title){
        var pi2 = Math.PI * 2;

        var cx = canvas.width / 2;
        var cy = canvas.height / 2;

        context.beginPath();
        context.moveTo(cx, cy);
        context.arc(cx, cy, radius, 0, pi2, false);
        context.fillStyle = getRandomColor();  
        context.fill();  

        var oldRadius = radius;
        var newRadius = 100;
        
        cx = cx + (oldRadius + newRadius);
        cy = cy;
        context.beginPath();
        context.moveTo(cx, cy);
        context.arc(cx, cy, newRadius, 0, pi2, false);
        context.fillStyle = getRandomColor();  
        context.fill();

        oldRadius = newRadius;
        newRadius = 20;

        cx = cx + (oldRadius + newRadius);
        cy = cy;
        context.beginPath();
        context.moveTo(cx, cy);
        context.arc(cx, cy, newRadius, 0, pi2, false);
        context.fillStyle = getRandomColor();  
        context.fill();

        oldRadius = newRadius;
        newRadius = 200;

        cx = cx + (oldRadius + newRadius);
        cy = cy;
        context.beginPath();
        context.moveTo(cx, cy);
        context.arc(cx, cy, newRadius, 0, pi2, false);
        context.fillStyle = getRandomColor();  
        context.fill();
        
        // context.fillStyle = 'white';
        // context.textAlign = 'center';
        // context.fillText(title, centerX, centerY);          
    }
});
