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

Author(s): Junes Najah, Thomas Riedmaier
*/

function deleteSetting(projId, settingId) {
    var deleteUrl = "/projects/" + projId + "/delSetting/" + settingId;
    window.location.replace(deleteUrl);
}

function deleteModule(projId, moduleId) {
    var deleteUrl = "/projects/" + projId + "/delModule/" + moduleId;
    window.location.replace(deleteUrl);
}

function deleteLocation(projId, projLocation) {
    var deleteUrl = "/projects/" + projId + "/delLocation/" + projLocation;
    window.location.replace(deleteUrl);
}

function updateSetting(projId, settingId, loopIndex) {
    var changeUrl = "/projects/" + projId + "/changeSetting/" + settingId;
    var data = { "SettingValue": document.getElementById("SettingValue-"+settingId).value };
    var rowIdSuccess = "#successIcon" + loopIndex;
    var rowIdError = "#errorIcon" + loopIndex;

    $.ajax({
        url: changeUrl,
        type: 'POST',
        data: JSON.stringify(data),
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: false,
        success: function(response) {
            if(response["status"] == "OK")
                responseMessage(rowIdSuccess);
            else
                responseMessage(rowIdError);
        }
    });
}

function responseMessage(rowId){
    $(rowId).css('visibility','visible'); 
    setTimeout(function() {
        $(rowId).fadeOut("slow", function(){
            $(rowId).css('visibility','hidden');
            $(rowId).show();
        });
        }, 1000);
}

function updateTypeSetting(projId, settingId, typeIDs, name) {
    var typeValues = [];
    var postfix = "";
    typeIDs.forEach(function(typeId){
        typeValue = parseInt($("#" + typeId).val());
        typeValues.push(typeValue);
    });
    if(validate(typeValues)){
        if(name == "mutator")
            $('#errorMessageOfMutator').hide();
        else
            $('#errorMessageOfEvaluator').hide();
        var changeUrl = "/projects/" + projId + "/changeSetting/" + settingId;
        var settingValue = "";
        typeValues.forEach(function(typeValue, i){
            postfix = i == (typeValues.length-1) ? "" : "|"; 
            settingValue += typeIDs[i] + "=" + typeValue + postfix; 
        });
        var data = { "SettingValue":  settingValue};

        $.ajax({
            url: changeUrl,
            type: 'POST',
            data: JSON.stringify(data),
            contentType: 'application/json; charset=utf-8',
            dataType: 'json',
            async: false,
            success: function(response) {
                if(response["status"] == "OK")
                    console.log("success");
                else
                    console.log("error");            
            }
        });
        
        location.reload();
    }else{        
        if(name == "mutator")
            $('#errorMessageOfMutator').show();
        else
            $('#errorMessageOfEvaluator').show();
    }
}

function validate(sequence){
    var sum = sequence.reduce(function(pv, cv) { return pv + cv; }, 0);
    return sum == 100;
}
