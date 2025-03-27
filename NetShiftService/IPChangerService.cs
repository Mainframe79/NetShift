using System.Diagnostics;
using System.ServiceModel;
using Microsoft.Win32;
using IPChanger.Models;
using System.ServiceProcess;
using System;
using System.Management;
using System.Linq;
using System.Net.NetworkInformation;
using System.ServiceModel.Description;

namespace IPChangerService
{
    [ServiceBehavior(InstanceContextMode = InstanceContextMode.Single, IncludeExceptionDetailInFaults = true)]
    public partial class IPChangerService : ServiceBase, IIPChangerService
    {
        private ServiceHost _host;
        private readonly string _logDir = System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "NetShift", "Logs");

        public IPChangerService()
        {
            InitializeComponent();
            _host = new ServiceHost(this, new Uri("net.pipe://localhost/NetShiftService"));
            _host.AddServiceEndpoint(typeof(IIPChangerService), new NetNamedPipeBinding(), "");
            var behavior = _host.Description.Behaviors.Find<ServiceDebugBehavior>();
            if (behavior == null)
            {
                behavior = new ServiceDebugBehavior();
                _host.Description.Behaviors.Add(behavior);
            }
            behavior.IncludeExceptionDetailInFaults = true;

            // Ensure the log directory exists
            if (!System.IO.Directory.Exists(_logDir))
            {
                System.IO.Directory.CreateDirectory(_logDir);
            }
        }

        protected override void OnStart(string[] args)
        {
            _host.Open();
        }

        protected override void OnStop()
        {
            _host.Close();
        }

        public void SetStaticIP(Preset preset)
        {
            try
            {
                if (preset == null || string.IsNullOrEmpty(preset.Name))
                {
                    throw new ArgumentException("Preset or interface name cannot be null or empty.");
                }

                if (string.IsNullOrEmpty(preset.IpAddress) || !System.Net.IPAddress.TryParse(preset.IpAddress, out _))
                {
                    throw new ArgumentException($"Invalid IP address: {preset.IpAddress ?? "null"}");
                }

                if (string.IsNullOrEmpty(preset.SubnetMask) || !System.Net.IPAddress.TryParse(preset.SubnetMask, out _))
                {
                    throw new ArgumentException($"Invalid subnet mask: {preset.SubnetMask ?? "null"}");
                }

                if (!string.IsNullOrEmpty(preset.Gateway) && !System.Net.IPAddress.TryParse(preset.Gateway, out _))
                {
                    throw new ArgumentException($"Invalid gateway: {preset.Gateway}");
                }

                if (!string.IsNullOrEmpty(preset.Dns) && !System.Net.IPAddress.TryParse(preset.Dns, out _))
                {
                    throw new ArgumentException($"Invalid DNS: {preset.Dns}");
                }

                bool isAdapterActive = IsAdapterActive(preset.Name);

                if (isAdapterActive)
                {
                    string command = $"interface ipv4 set address name=\"{preset.Name}\" source=static address={preset.IpAddress} mask={preset.SubnetMask}";
                    if (!string.IsNullOrEmpty(preset.Gateway))
                    {
                        command += $" gateway={preset.Gateway} gwmetric=1";
                    }
                    else
                    {
                        command += " gateway=none";
                    }

                    ProcessStartInfo psi = new ProcessStartInfo
                    {
                        FileName = "netsh",
                        Arguments = command,
                        UseShellExecute = false,
                        RedirectStandardOutput = true,
                        RedirectStandardError = true,
                        CreateNoWindow = true
                    };

                    try
                    {
                        using (Process process = Process.Start(psi)!)
                        {
                            process.WaitForExit();

                            string output = process.StandardOutput.ReadToEnd();
                            string error = process.StandardError.ReadToEnd();

                            System.IO.File.WriteAllText(System.IO.Path.Combine(_logDir, "netsh_output_static.log"), $"Command: {command}\nOutput: {output}\nError: {error}");

                            if (process.ExitCode != 0)
                            {
                                throw new Exception($"netsh command failed: {error}\nOutput: {output}");
                            }
                        }
                    }
                    catch (Exception ex) when (ex.Message.Contains("The following command was not found") || ex.Message.Contains("The syntax supplied for this command is not valid"))
                    {
                        command = $"interface ip set address name=\"{preset.Name}\" source=static address={preset.IpAddress} mask={preset.SubnetMask}";
                        if (!string.IsNullOrEmpty(preset.Gateway))
                        {
                            command += $" gateway={preset.Gateway} gwmetric=1";
                        }
                        else
                        {
                            command += " gateway=none";
                        }

                        psi.Arguments = command;

                        using (Process process = Process.Start(psi)!)
                        {
                            process.WaitForExit();

                            string output = process.StandardOutput.ReadToEnd();
                            string error = process.StandardError.ReadToEnd();

                            System.IO.File.WriteAllText(System.IO.Path.Combine(_logDir, "netsh_output_static.log"), $"Command: {command}\nOutput: {output}\nError: {error}");

                            if (process.ExitCode != 0)
                            {
                                throw new Exception($"netsh command failed: {error}\nOutput: {output}");
                            }
                        }
                    }
                }
                else
                {
                    SetAdapterToStaticIpViaRegistry(preset);
                }

                if (!string.IsNullOrEmpty(preset.Dns))
                {
                    if (isAdapterActive)
                    {
                        string command = $"interface ipv4 set dns name=\"{preset.Name}\" source=static address={preset.Dns}";

                        ProcessStartInfo psi = new ProcessStartInfo
                        {
                            FileName = "netsh",
                            Arguments = command,
                            UseShellExecute = false,
                            RedirectStandardOutput = true,
                            RedirectStandardError = true,
                            CreateNoWindow = true
                        };

                        try
                        {
                            using (Process process = Process.Start(psi)!)
                            {
                                process.WaitForExit();

                                string output = process.StandardOutput.ReadToEnd();
                                string error = process.StandardError.ReadToEnd();

                                System.IO.File.AppendAllText(System.IO.Path.Combine(_logDir, "netsh_output_static.log"), $"\nCommand: {command}\nOutput: {output}\nError: {error}");

                                if (process.ExitCode != 0)
                                {
                                    throw new Exception($"netsh command failed: {error}\nOutput: {output}");
                                }
                            }
                        }
                        catch (Exception ex) when (ex.Message.Contains("The following command was not found") || ex.Message.Contains("The syntax supplied for this command is not valid"))
                        {
                            command = $"interface ip set dns name=\"{preset.Name}\" source=static address={preset.Dns}";

                            psi.Arguments = command;

                            using (Process process = Process.Start(psi)!)
                            {
                                process.WaitForExit();

                                string output = process.StandardOutput.ReadToEnd();
                                string error = process.StandardError.ReadToEnd();

                                System.IO.File.AppendAllText(System.IO.Path.Combine(_logDir, "netsh_output_static.log"), $"\nCommand: {command}\nOutput: {output}\nError: {error}");

                                if (process.ExitCode != 0)
                                {
                                    throw new Exception($"netsh command failed: {error}\nOutput: {output}");
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                System.IO.File.WriteAllText(System.IO.Path.Combine(_logDir, "service_error.log"), $"SetStaticIP failed: {ex.Message}\n{ex.StackTrace}");
                throw;
            }
        }

        public void ResetToDhcp(string adapterName)
        {
            try
            {
                bool isAdapterActive = IsAdapterActive(adapterName);

                if (isAdapterActive)
                {
                    string command = $"interface ipv4 set address name=\"{adapterName}\" source=dhcp";
                    try
                    {
                        ProcessStartInfo psi = new ProcessStartInfo
                        {
                            FileName = "netsh",
                            Arguments = command,
                            UseShellExecute = false,
                            RedirectStandardOutput = true,
                            RedirectStandardError = true,
                            CreateNoWindow = true
                        };

                        using (Process process = Process.Start(psi)!)
                        {
                            process.WaitForExit();

                            string output = process.StandardOutput.ReadToEnd();
                            string error = process.StandardError.ReadToEnd();

                            System.IO.File.WriteAllText(System.IO.Path.Combine(_logDir, "netsh_output_dhcp.log"), $"Command: {command}\nOutput: {output}\nError: {error}");

                            if (process.ExitCode != 0)
                            {
                                throw new Exception($"netsh command failed: {error}\nOutput: {output}");
                            }
                        }
                    }
                    catch (Exception ex) when (ex.Message.Contains("The interface is not configurable"))
                    {
                        SetAdapterToDhcpViaRegistry(adapterName);
                    }

                    command = $"interface ipv4 set dns name=\"{adapterName}\" source=dhcp";
                    try
                    {
                        ProcessStartInfo psi = new ProcessStartInfo
                        {
                            FileName = "netsh",
                            Arguments = command,
                            UseShellExecute = false,
                            RedirectStandardOutput = true,
                            RedirectStandardError = true,
                            CreateNoWindow = true
                        };

                        using (Process process = Process.Start(psi)!)
                        {
                            process.WaitForExit();

                            string output = process.StandardOutput.ReadToEnd();
                            string error = process.StandardError.ReadToEnd();

                            System.IO.File.AppendAllText(System.IO.Path.Combine(_logDir, "netsh_output_dhcp.log"), $"\nCommand: {command}\nOutput: {output}\nError: {error}");

                            if (process.ExitCode != 0)
                            {
                                throw new Exception($"netsh command failed: {error}\nOutput: {output}");
                            }
                        }
                    }
                    catch (Exception ex) when (ex.Message.Contains("The interface is not configurable"))
                    {
                        // DNS settings are already handled by SetAdapterToDhcpViaRegistry
                    }
                }
                else
                {
                    SetAdapterToDhcpViaRegistry(adapterName);
                }
            }
            catch (Exception ex)
            {
                System.IO.File.WriteAllText(System.IO.Path.Combine(_logDir, "service_error.log"), $"ResetToDhcp failed: {ex.Message}\n{ex.StackTrace}");
                throw;
            }
        }

        private bool IsAdapterActive(string adapterName)
        {
            var networkInterface = NetworkInterface.GetAllNetworkInterfaces()
                .FirstOrDefault(ni => ni.Name == adapterName &&
                                     ni.OperationalStatus == OperationalStatus.Up &&
                                     ni.NetworkInterfaceType != NetworkInterfaceType.Loopback);

            return networkInterface != null;
        }

        private void SetAdapterToStaticIpViaRegistry(Preset preset)
        {
            try
            {
                string adapterGuid = null;
                using (var searcher = new ManagementObjectSearcher($"SELECT * FROM Win32_NetworkAdapter WHERE NetConnectionID = '{preset.Name}'"))
                {
                    var adapter = searcher.Get().Cast<ManagementObject>().FirstOrDefault();
                    if (adapter == null)
                    {
                        throw new Exception($"Adapter {preset.Name} not found.");
                    }
                    adapterGuid = adapter["GUID"]?.ToString();
                    if (string.IsNullOrEmpty(adapterGuid))
                    {
                        string deviceId = adapter["DeviceID"]?.ToString();
                        if (string.IsNullOrEmpty(deviceId))
                        {
                            throw new Exception($"Could not find DeviceID for adapter {preset.Name}.");
                        }

                        using (var configSearcher = new ManagementObjectSearcher($"SELECT * FROM Win32_NetworkAdapterConfiguration WHERE Index = '{deviceId}'"))
                        {
                            var config = configSearcher.Get().Cast<ManagementObject>().FirstOrDefault();
                            if (config == null)
                            {
                                throw new Exception($"Could not find Win32_NetworkAdapterConfiguration for adapter {preset.Name} with DeviceID {deviceId}.");
                            }
                            adapterGuid = config["SettingID"]?.ToString();
                        }
                    }
                }

                if (string.IsNullOrEmpty(adapterGuid))
                {
                    throw new Exception($"Could not find GUID for adapter {preset.Name}.");
                }

                string registryPath = $@"SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces\{adapterGuid}";
                using (RegistryKey key = Registry.LocalMachine.OpenSubKey(registryPath, writable: true) ?? Registry.LocalMachine.CreateSubKey(registryPath))
                {
                    if (key == null)
                    {
                        throw new Exception($"Could not open or create registry key for adapter {preset.Name} at {registryPath}.");
                    }

                    key.SetValue("EnableDHCP", 0, RegistryValueKind.DWord);
                    key.SetValue("IPAddress", new string[] { preset.IpAddress }, RegistryValueKind.MultiString);
                    key.SetValue("SubnetMask", new string[] { preset.SubnetMask }, RegistryValueKind.MultiString);
                    key.SetValue("DefaultGateway", string.IsNullOrEmpty(preset.Gateway) ? new string[] { "" } : new string[] { preset.Gateway }, RegistryValueKind.MultiString);

                    if (!string.IsNullOrEmpty(preset.Dns))
                    {
                        key.SetValue("NameServer", preset.Dns, RegistryValueKind.String);
                    }
                    else
                    {
                        key.SetValue("NameServer", "", RegistryValueKind.String);
                    }
                }
            }
            catch (Exception ex)
            {
                throw new Exception($"Failed to set adapter {preset.Name} to static IP via registry: {ex.Message}", ex);
            }
        }

        private void SetAdapterToDhcpViaRegistry(string adapterName)
        {
            try
            {
                string adapterGuid = null;
                using (var searcher = new ManagementObjectSearcher($"SELECT * FROM Win32_NetworkAdapter WHERE NetConnectionID = '{adapterName}'"))
                {
                    var adapter = searcher.Get().Cast<ManagementObject>().FirstOrDefault();
                    if (adapter == null)
                    {
                        throw new Exception($"Adapter {adapterName} not found.");
                    }
                    adapterGuid = adapter["GUID"]?.ToString();
                }

                if (string.IsNullOrEmpty(adapterGuid))
                {
                    throw new Exception($"Could not find GUID for adapter {adapterName}.");
                }

                string registryPath = $@"SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces\{adapterGuid}";
                using (RegistryKey key = Registry.LocalMachine.OpenSubKey(registryPath, writable: true))
                {
                    if (key == null)
                    {
                        throw new Exception($"Could not find registry key for adapter {adapterName}.");
                    }

                    key.SetValue("EnableDHCP", 1, RegistryValueKind.DWord);
                    key.SetValue("IPAddress", new string[] { "0.0.0.0" }, RegistryValueKind.MultiString);
                    key.SetValue("SubnetMask", new string[] { "0.0.0.0" }, RegistryValueKind.MultiString);
                    key.SetValue("DefaultGateway", new string[] { "" }, RegistryValueKind.MultiString);
                    key.SetValue("NameServer", "", RegistryValueKind.String);
                }
            }
            catch (Exception ex)
            {
                throw new Exception($"Failed to set adapter {adapterName} to DHCP via registry: {ex.Message}", ex);
            }
        }
    }
}