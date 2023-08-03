# Приветствие
Write-Host "Добро пожаловать в PowerShell!"

# Вывод информации о текущем пользователе
$user = $env:USERNAME
Write-Host "Вы вошли как: $user"

# Вывод информации об операционной системе
$os = Get-WmiObject Win32_OperatingSystem
Write-Host "Операционная система: $($os.Caption) $($os.Version)"