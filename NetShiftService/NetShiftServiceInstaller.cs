using System.ComponentModel;
using System.Configuration.Install;
using System.ServiceProcess;

namespace NetShiftService
{
    [RunInstaller(true)]
    public class NetShiftServiceInstaller : Installer
    {
        private readonly ServiceProcessInstaller _processInstaller;
        private readonly ServiceInstaller _serviceInstaller;

        public NetShiftServiceInstaller()
        {
            _processInstaller = new ServiceProcessInstaller
            {
                Account = ServiceAccount.User,
                Username = @".\test", // Replace with your service account (e.g., "J61\serviceaccount")
                Password = "ShortPass" // Replace with the service account password
            };

            _serviceInstaller = new ServiceInstaller
            {
                ServiceName = "NetShiftService",
                DisplayName = "NetShift Service",
                Description = "Service to handle IP address changes for the NetShift application.",
                StartType = ServiceStartMode.Automatic
            };

            Installers.Add(_processInstaller);
            Installers.Add(_serviceInstaller);
        }
    }
}