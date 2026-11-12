local account = 1
local mailMeasures, labels, inboxes, seen = {}, {}, {}, {}
local resources, recentMinutes, showDetails
local configured = {}
local author, subject, tooltip = '', '', 'Checking inbox'

local entities = {amp = '&', lt = '<', gt = '>', quot = '"', apos = "'"}

local function Utf8(code)
    if not code or code < 32 or code > 0x10FFFF or (code >= 0xD800 and code <= 0xDFFF) then
        return nil
    end
    if code < 128 then
        return string.char(code)
    elseif code < 2048 then
        return string.char(192 + math.floor(code / 64), 128 + code % 64)
    elseif code < 65536 then
        return string.char(224 + math.floor(code / 4096), 128 + math.floor(code / 64) % 64,
            128 + code % 64)
    end
    return string.char(240 + math.floor(code / 262144), 128 + math.floor(code / 4096) % 64,
        128 + math.floor(code / 64) % 64, 128 + code % 64)
end

local function DecodeXml(value)
    value = value:gsub('^<!%[CDATA%[(.*)%]%]>$', '%1')
    return (value:gsub('&([^&;]+);', function(entity)
        local hex = entity:match('^#x(%x+)$')
        local decimal = entity:match('^#(%d+)$')
        return entities[entity] or Utf8(hex and tonumber(hex, 16) or tonumber(decimal or ''))
    end))
end

local function Field(xml, name)
    return xml:match('<' .. name .. '%s*>(.-)</' .. name .. '>') or
        xml:match('<' .. name .. '%s+[^>]*>(.-)</' .. name .. '>')
end

local function IsRecent(timestamp)
    local year, month, day, hour, minute, second = timestamp:match(
        '^(%d%d%d%d)%-(%d%d)%-(%d%d)T(%d%d):(%d%d):(%d%d)')
    if not year then return false end
    local itemTime = os.time({year = year, month = month, day = day, hour = hour,
        min = minute, sec = second, isdst = false})
    local utc = os.date('!*t')
    utc.isdst = false
    local minutes = os.difftime(os.time(utc), itemTime) / 60
    return minutes >= 0 and minutes < recentMinutes
end

local function StatePath(index)
    return resources .. 'Newest' .. index .. '.txt'
end

local function Remember(index, key)
    seen[index] = key
    local file = io.open(StatePath(index), 'w')
    if file then
        file:write(key, '\n')
        file:close()
    end
end

local function Draw(countText, hasMail, recent)
    SKIN:Bang('!SetOption', 'MeterImage', 'GreyScale', hasMail and '0' or '1')
    SKIN:Bang('!MoveMeter', '154', recent and '14' or '22', 'MeterCount')
    SKIN:Bang('!SetOption', 'MeterCount', 'Text', countText)
    SKIN:Bang('!SetOption', 'MeterCount', 'FontColor', hasMail and '#ValueColor#' or '#LabelColor#')
    SKIN:Bang('!UpdateMeter', '*')
    SKIN:Bang('!Redraw')
end

function Initialize()
    resources = SKIN:GetVariable('@')
    recentMinutes = tonumber(SKIN:GetVariable('RecentMinutes')) or 15
    showDetails = SKIN:GetVariable('ShowMessageDetails') ~= '0'
    for index = 1, 2 do
        mailMeasures[index] = SKIN:GetMeasure('MeasureMail' .. index)
        labels[index] = SKIN:GetVariable('GMailLabel' .. index)
        inboxes[index] = SKIN:GetVariable('InboxURL' .. index)
        local user = SKIN:GetVariable('GMailID' .. index, '')
        local password = SKIN:GetVariable('GMailPassword' .. index, '')
        configured[index] = user ~= '' and password ~= '' and
            user ~= 'YOUR_GMAIL_ADDRESS' and password ~= 'YOUR_GMAIL_APP_PASSWORD'
        local file = io.open(StatePath(index), 'r')
        if file then
            seen[index] = file:read('*l')
            file:close()
        end
    end
end

function GetAuthor() return showDetails and author or 'New unread message' end
function GetSubject() return showDetails and subject or '' end
function GetTooltip() return tooltip end

function CheckMail()
    if not configured[account] then
        tooltip = 'Set this account in @Resources\\GMailNotify.inc, then refresh the skin.'
        Draw('Set credentials', false, false)
        return
    end
    SKIN:Bang('!EnableMeasure', 'MeasureMail' .. account)
    SKIN:Bang('!CommandMeasure', 'MeasureMail' .. account, 'Update')
end

function SelectAccount(index)
    if index == account then
        CheckMail()
        return
    end
    SKIN:Bang('!DisableMeasure', 'MeasureMail' .. account)
    account = index
    author, subject, tooltip = '', '', 'Checking inbox'
    SKIN:Bang('!SetVariable', 'ActiveLabel', labels[index])
    SKIN:Bang('!SetVariable', 'ActiveInboxURL', inboxes[index])
    Draw('Checking', false, false)
    CheckMail()
end

function FeedError(index)
    if index ~= account then return end
    tooltip = 'Could not read the Gmail feed. Check the account settings in @Resources.'
    Draw('Feed error', false, false)
end

function ProcessMail(index)
    if index ~= account then return end
    local feed = mailMeasures[index]:GetStringValue()
    local unread = tonumber(Field(feed, 'fullcount'))
    if not unread then
        FeedError(index)
        return
    end
    if unread == 0 then
        author, subject, tooltip = '', '', 'No unread in Inbox'
        if seen[index] == nil then Remember(index, '') end
        Draw('No Unread', false, false)
        return
    end

    local entry = Field(feed, 'entry')
    if not entry then
        tooltip = unread .. ' unread in Inbox'
        Draw(unread .. ' Unread', true, false)
        return
    end

    local sender = Field(entry, 'author') or ''
    author = DecodeXml(Field(sender, 'name') or Field(sender, 'email') or 'Unknown sender')
    subject = DecodeXml(Field(entry, 'title') or '(No subject)')
    local timestamp = Field(entry, 'modified') or Field(entry, 'issued') or ''
    local id = Field(entry, 'id') or ''
    local key = timestamp .. ' ' .. id
    local recent = IsRecent(timestamp)
    tooltip = unread .. ' unread in Inbox\n' .. author .. '\n' .. subject
    Draw(unread .. ' Unread' .. (recent and '#CRLF#Recent' or ''), true, recent)

    if key == ' ' or key == seen[index] then return end
    local previous = seen[index]
    local previousTimestamp = previous and previous:match('^(%S+) ')
    if previousTimestamp and timestamp < previousTimestamp then return end

    Remember(index, key)
    if previous ~= nil then
        SKIN:Bang('!CommandMeasure', 'MeasureToast', 'Show')
    end
end
