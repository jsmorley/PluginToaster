local message = ''
local input, toaster

function Initialize()
    input = SKIN:GetMeasure('MeasureInput')
    toaster = SKIN:GetMeasure('MeasureToast')
end

local function SetStatus(text)
    SKIN:Bang('!SetOption', 'MeterStatus', 'Text', text)
    SKIN:Bang('!UpdateMeter', 'MeterStatus')
    SKIN:Bang('!Redraw')
end

function GetMessage()
    return message
end

function Send()
    -- Read InputText directly so quotes and brackets stay part of the message.
    message = input:GetStringValue():match('^%s*(.-)%s*$')
    if message == '' then
        SetStatus('Type a message first.')
        return
    end
    SKIN:Bang('!CommandMeasure', 'MeasureToast', 'Show')
    SKIN:Bang('!UpdateMeasure', 'MeasureToast')
    SKIN:Bang('!CommandMeasure', 'MeasureForm', 'Report()')
end

function Report()
    local result = toaster:GetValue()
    if result == 1 then
        SetStatus('Sent to Windows.')
    elseif result == 2 then
        SetStatus('Repeat suppressed. Wait five seconds.')
    else
        SetStatus('Send failed. See Rainmeter’s log.')
    end
end
