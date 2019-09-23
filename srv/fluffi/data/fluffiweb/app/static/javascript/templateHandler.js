§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Junes Najah, Thomas Riedmaier
§§*/
§§
§§var config = (function() {
§§    var config = null;
§§    $.ajax({
§§        'async': false,
§§        'global': false,
§§        'url': "/static/config.json",
§§        'dataType': "json",
§§        'success': function (data) {
§§            config = data;
§§        }
§§    });
§§    return config;
§§})();
§§
§§var templates = config["Templates"];
§§
§§function fillTemplate(name){    
§§    if(name != "None"){
§§        templates.forEach(function(template){
§§            if(template[name]){
§§                template[name].forEach(function(setting){
§§                    if(setting["optionNames"]){
§§                        setting["optionNames"].forEach(function(optionName, i){
§§                            $('#addOptionRow').click();
§§                            $('#' + (i+1) + "_optionname").val(optionName);
§§                        });
§§                    }
§§                    if(setting["optionValues"]){
§§                        setting["optionValues"].forEach(function(optionValue, i){
§§                            $('#' + (i+1) + "_optionvalue").val(optionValue);
§§                        });
§§                    }
§§                    if(setting["targetCMDLine"]){
§§                        $("#targetCMDLine").val(setting["targetCMDLine"]);
§§                    }
§§                });
§§            }
§§        }); 
§§    }
§§    else {
§§        location.reload();
§§    }
§§
§§}
