'Documentation: Logging.LogTraceListener
Namespace Logging

    ''' <summary>
    ''' Private implementation of a log file
    ''' </summary>
    ''' <remarks></remarks>
    Public Class LogTraceListener
        Inherits TraceListener

        ''' <summary>
        ''' Used for thread safe operations
        ''' </summary>
        ''' <remarks></remarks>
        Private m_LogFileMutex As Threading.Mutex = Nothing

        ''' <summary>
        ''' LogFileName = &lt;User Directory&gt;\RenEvo Software &amp; Designs\The Dead Six Tools\&lt;Assembly File Version&gt;\&lt;AssemblyName&gt;.log
        ''' </summary>
        ''' <remarks></remarks>
        Private m_LogFileName As String = String.Format("{0}\RenEvo Software & Designs\The Dead Six Tools\{1}\{2}.log", Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), DirectCast(Reflection.Assembly.GetExecutingAssembly().GetCustomAttributes(GetType(System.Reflection.AssemblyFileVersionAttribute), True)(0), System.Reflection.AssemblyFileVersionAttribute).Version, My.Application.Log.DefaultFileLogWriter.BaseFileName)

        Public Sub New(ByVal Data As String)
            Me.New()
            If Data.Length > 0 Then
                m_LogFileName = Data
                SetupLogFile()
            End If
        End Sub

        Public Sub New()
            m_LogFileMutex = New Threading.Mutex(False, New Guid().ToString())
            SetupLogFile()
        End Sub

        Private Sub SetupLogFile()
            m_LogFileMutex.WaitOne()
            System.IO.Directory.CreateDirectory(System.IO.Path.GetDirectoryName(m_LogFileName))
            If System.IO.File.Exists(m_LogFileName) Then
                System.IO.File.Delete(m_LogFileName)
            End If
            m_LogFileMutex.ReleaseMutex()
        End Sub

        Public Overloads Overrides Sub Write(ByVal message As String)
            m_LogFileMutex.WaitOne()
            System.IO.File.AppendAllText(m_LogFileName, message)
            m_LogFileMutex.ReleaseMutex()
        End Sub

        Public Overloads Overrides Sub WriteLine(ByVal message As String)
            Me.Write(FormatMessage(message) + Environment.NewLine)
        End Sub

        Protected Overrides Sub Dispose(ByVal disposing As Boolean)
            m_LogFileMutex = Nothing
            MyBase.Dispose(disposing)
        End Sub

        Private Function FormatMessage(ByVal message As String) As String
            Return String.Format("[{1:00}.{2:00}.{0:0000} {3:00}:{4:00}:{5:00}] {6}", Now.Year, Now.Month, Now.Day, Now.Hour, Now.Minute, Now.Second, message)
        End Function

    End Class

End Namespace