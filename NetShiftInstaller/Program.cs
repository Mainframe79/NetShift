using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using Microsoft.Win32;

namespace NetShiftInstaller // Update the namespace to match the new project name
{
    class Program
    {
        static void Main(string[] args)
        {
            bool silent = args.Any(arg => arg.Equals("/silent", StringComparison.OrdinalIgnoreCase));
            string tempDir = null;
            try
            {
                if (!silent)
                {
                    Console.WriteLine("Starting NetShift installation...");
                }

                // Create a temporary directory to extract resources
                tempDir = Path.Combine(Path.GetTempPath(), "NetShiftInstaller");
                if (!silent)
                {
                    Console.WriteLine($"Temporary directory path: {tempDir}");
                }
                if (Directory.Exists(tempDir))
                {
                    if (!silent)
                    {
                        Console.WriteLine("Temporary directory already exists, deleting...");
                    }
                    Directory.Delete(tempDir, true);
                }
                if (!silent)
                {
                    Console.WriteLine("Creating temporary directory...");
                }
                Directory.CreateDirectory(tempDir);
                if (!silent)
                {
                    Console.WriteLine("Temporary directory created.");
                }

                // Extract the embedded resources
                if (!silent)
                {
                    Console.WriteLine("Extracting NetShiftSetup.msi...");
                }
                ExtractResource("NetShiftInstaller.NetShiftSetup.msi", tempDir, "NetShiftSetup.msi", silent); // Update the resource name if necessary
                if (!silent)
                {
                    Console.WriteLine("Extracting NDP48-x86-x64-AllOS-ENU.exe...");
                }
                ExtractResource("NetShiftInstaller.NDP48-x86-x64-AllOS-ENU.exe", tempDir, "NDP48-x86-x64-AllOS-ENU.exe", silent);
                if (!silent)
                {
                    Console.WriteLine("Extracting windowsdesktop-runtime-8.0.14-win-x64.exe...");
                }
                ExtractResource("NetShiftInstaller.windowsdesktop-runtime-8.0.14-win-x64.exe", tempDir, "windowsdesktop-runtime-8.0.14-win-x64.exe", silent);
                if (!silent)
                {
                    Console.WriteLine("All resources extracted successfully.");
                }

                // Check and install .NET Framework 4.8
                if (!IsNetFx48Installed())
                {
                    if (!silent)
                    {
                        Console.WriteLine("Installing .NET Framework 4.8...");
                    }
                    InstallPackage(Path.Combine(tempDir, "NDP48-x86-x64-AllOS-ENU.exe"), "/q /norestart", ".NET Framework 4.8", silent);
                }
                else if (!silent)
                {
                    Console.WriteLine(".NET Framework 4.8 is already installed.");
                }

                // Check and install .NET 8 Desktop Runtime
                if (!IsNet8DesktopRuntimeInstalled())
                {
                    if (!silent)
                    {
                        Console.WriteLine("Installing .NET 8 Desktop Runtime...");
                    }
                    InstallPackage(Path.Combine(tempDir, "windowsdesktop-runtime-8.0.14-win-x64.exe"), "/install /quiet /norestart", ".NET 8 Desktop Runtime", silent);
                }
                else if (!silent)
                {
                    Console.WriteLine(".NET 8 Desktop Runtime is already installed.");
                }

                // Install the NetShift MSI
                if (!silent)
                {
                    Console.WriteLine("Installing NetShift MSI...");
                }
                string msiPath = Path.Combine(tempDir, "NetShiftSetup.msi");
                if (!silent)
                {
                    Console.WriteLine($"MSI Path: {msiPath}");
                }
                if (!File.Exists(msiPath))
                {
                    throw new Exception($"MSI file not found at: {msiPath}");
                }
                InstallPackage("msiexec.exe", $"/i \"{msiPath}\" /quiet /norestart /l*v \"{Path.Combine(tempDir, "NetShiftSetup.log")}\"", "NetShift MSI", silent);

                if (!silent)
                {
                    Console.WriteLine("NetShift installation completed successfully.");
                }
            }
            catch (Exception ex)
            {
                if (!silent)
                {
                    Console.WriteLine($"Installation failed: {ex.Message}");
                }
                Environment.Exit(1);
            }
            finally
            {
                // Clean up the temporary directory
                if (tempDir != null && Directory.Exists(tempDir))
                {
                    try
                    {
                        Directory.Delete(tempDir, true);
                    }
                    catch (Exception ex)
                    {
                        if (!silent)
                        {
                            Console.WriteLine($"Warning: Could not clean up temporary directory: {ex.Message}");
                        }
                    }
                }
            }
        }

        static void ExtractResource(string resourceName, string targetDir, string targetFileName, bool silent)
        {
            if (!silent)
            {
                Console.WriteLine($"Attempting to extract resource: {resourceName}");
            }
            using (Stream resourceStream = Assembly.GetExecutingAssembly().GetManifestResourceStream(resourceName))
            {
                if (resourceStream == null)
                {
                    throw new Exception($"Resource {resourceName} not found in the assembly.");
                }

                string targetPath = Path.Combine(targetDir, targetFileName);
                if (!silent)
                {
                    Console.WriteLine($"Extracting to: {targetPath}");
                }
                using (FileStream fileStream = new FileStream(targetPath, FileMode.Create, FileAccess.Write))
                {
                    resourceStream.CopyTo(fileStream);
                }
                if (!silent)
                {
                    Console.WriteLine($"Successfully extracted: {targetPath}");
                }
            }
        }

        static bool IsNetFx48Installed()
        {
            using (RegistryKey key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full"))
            {
                if (key != null)
                {
                    int release = (int)key.GetValue("Release", 0);
                    return release >= 528040; // 528040 is .NET Framework 4.8
                }
                return false;
            }
        }

        static bool IsNet8DesktopRuntimeInstalled()
        {
            using (RegistryKey key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\dotnet\Setup\InstalledVersions\x64\sharedfx\Microsoft.WindowsDesktop.App"))
            {
                if (key != null)
                {
                    return key.GetValue("8.0.14") != null;
                }
                return false;
            }
        }

        static void InstallPackage(string installerPath, string arguments, string packageName, bool silent)
        {
            ProcessStartInfo startInfo = new ProcessStartInfo
            {
                FileName = installerPath,
                Arguments = arguments,
                UseShellExecute = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                CreateNoWindow = true
            };

            using (Process process = Process.Start(startInfo))
            {
                process.WaitForExit();

                if (process.ExitCode == 0)
                {
                    if (!silent)
                    {
                        Console.WriteLine($"{packageName} installed successfully.");
                    }
                }
                else if (process.ExitCode == 3010)
                {
                    if (!silent)
                    {
                        Console.WriteLine($"{packageName} installed successfully, but a reboot is required.");
                        Console.WriteLine("Please reboot the machine and rerun the installer.");
                    }
                    Environment.Exit(3010);
                }
                else
                {
                    string output = process.StandardOutput.ReadToEnd();
                    string error = process.StandardError.ReadToEnd();
                    throw new Exception($"Failed to install {packageName}. Exit code: {process.ExitCode}. Output: {output} Error: {error}");
                }
            }
        }
    }
}