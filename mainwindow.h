#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QDir>
#include <QDateTime>

class QLineEdit;
class QWidget;
class QVBoxLayout;
class QTableWidget;
class QNetworkAccessManager;
class QNetworkReply;
class QTableWidgetItem;
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private slots:
	void on_browse_clicked();
	void on_net_reply(QNetworkReply* reply);

private:
    void SetupGUI();

    QWidget* widget;
	QVBoxLayout* main_layout;


    QLineEdit* game_path;
	QSettings* settings;
	QTableWidget* table;
	QNetworkAccessManager* net_man;

	void Information(QString s);

	QDir dir;

	void FindBuffs();

	void ParseLUA(const QString& file_path);
	void RequestRealmplayerData(const QString& name);

	void UpdateTable();

	const QString data_file_name = "OnyBuffCollecter.lua";
	const QString url_base = "http://www.realmplayers.com/CharacterViewer.aspx?realm=NRB&player=";

	class BuffData{
	public:
		BuffData(){}
		BuffData(const QString& n,QDateTime t) 
			: name(n),date_time(t),
			guild_itm(NULL){}

		QString name;
		QString guild;
		QDateTime date_time;

		QTableWidgetItem* guild_itm;
	};

	QMap<QString, BuffData> data_map;

	enum table_index{
		T_DATE,
		T_TIME,
		T_NAME,
		T_GUILD,
		T_NUM_COLS
	};
};

#endif // MAINWINDOW_H
