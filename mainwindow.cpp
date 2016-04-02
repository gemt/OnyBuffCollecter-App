#include "mainwindow.h"

#include <QSettings>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QWidget>
#include <QPushButton>

#include <QApplication>
#include <QCoreApplication>
#include <QRect>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <Qdebug>
#include <QMessageBox>
#include <QTableWidget>
#include <QLabel>
#include <QTableWidgetItem>
#include <QHeaderView>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    game_path(NULL)
{
	QCoreApplication::setOrganizationName("Phoenix");
	QCoreApplication::setOrganizationDomain("phoenix-guilde.shivtr.com");
	QCoreApplication::setApplicationName("OnyxiaBuffCollecter");
	settings = new QSettings();

    widget = new QWidget;
    setCentralWidget(widget);
	setWindowTitle("OnyBuffCollecter - lets collect Onyxia buffs!");
	main_layout = new QVBoxLayout();
	widget->setLayout(main_layout);
	
	QRect rec = QApplication::desktop()->screenGeometry();
	resize(rec.width()/4, rec.height()/4);

	net_man = new QNetworkAccessManager(widget);
	connect(net_man, SIGNAL(finished(QNetworkReply*)), this, SLOT(on_net_reply(QNetworkReply*)));
	SetupGUI();
}

MainWindow::~MainWindow()
{
    delete widget;
	delete settings;
}

void MainWindow::SetupGUI()
{	
	

	QFormLayout* input_layout = new QFormLayout();
	main_layout->addLayout(input_layout);
	

	QHBoxLayout* game_dir_layout = new QHBoxLayout();
	game_path = new QLineEdit();
	qDebug() << settings->value("game_dir");
	
	game_path->setText(settings->value("game_dir").toString());

	game_path->setToolTip("Select the directory where your Wow.exe file is");

	input_layout->addRow("Game Path", game_dir_layout);
	game_dir_layout->addWidget(game_path);
	QPushButton* browse_btn = new QPushButton("...");
	game_dir_layout->addWidget(browse_btn);

	connect(browse_btn, SIGNAL(clicked()), this, SLOT(on_browse_clicked()));
    
	QPushButton* load_btn = new QPushButton("Load data");
	input_layout->addWidget(load_btn);
	connect(load_btn, &QPushButton::clicked, [this](){
		FindBuffs();
		UpdateTable();
	});

	main_layout->addWidget(new QLabel("Discovered buffs:"));
	table = new QTableWidget();
	table->setColumnCount(T_NUM_COLS);
	QStringList labels;
	labels.insert(T_DATE, "Date");
	labels.insert(T_TIME, "Time");
	labels.insert(T_NAME, "Name");
	labels.insert(T_GUILD, "Guild");
	table->setHorizontalHeaderLabels(labels);
	table->horizontalHeader()->setStretchLastSection(true);
	table->setSortingEnabled(true);
	main_layout->addWidget(table);

	
}

void MainWindow::on_browse_clicked()
{

	QString ret = QFileDialog::getExistingDirectory(
		widget,
		"Select the directory where your Wow.exe file is. The dialog will only show folders.",
		settings->value("game_dir").toString(), 0);
		
	if (ret.isEmpty())
		return;
	dir.setCurrent(ret);
	if (dir.entryList().contains("WoW.exe")){
		game_path->setText(ret);
		settings->setValue("game_dir", ret);
	}
	else{
		Information("Could not find a Wow.exe file in the selected directory.");
	}
}

void MainWindow::on_net_reply(QNetworkReply* reply)
{
	if (reply->error() != QNetworkReply::NoError){
		qDebug() << reply->errorString();
		return;
	}

	QString data = reply->readAll();
	int start = data.indexOf("<ul class=\"breadcrumb\">");
	if (start == -1) return;
	int stop = data.indexOf("<!--/.breadcrumb -->");
	if (stop == -1) return;

	QString breadcrumb = data.mid(start, stop);
	if (breadcrumb.isEmpty())
		return;
	
	start = breadcrumb.indexOf("guild=");
	if (start == -1) return;
	breadcrumb = breadcrumb.right(breadcrumb.size() - start);
	stop = breadcrumb.indexOf("'>");
	if (stop == -1) return;
	breadcrumb = breadcrumb.left(stop);
	breadcrumb = breadcrumb.remove("guild=");
	breadcrumb = breadcrumb.trimmed();
	
	QString player_name = reply->url().toString().remove(url_base);
	if (data_map.contains(player_name)){
		data_map[player_name].guild = breadcrumb;

		if (data_map[player_name].guild_itm)
			data_map[player_name].guild_itm->setText(breadcrumb);
	}
}

void MainWindow::Information(QString s)
{
	QMessageBox msg;
	msg.setIcon(QMessageBox::Information);
	msg.setText(s);
	msg.show();
	msg.exec();
}

void MainWindow::FindBuffs()
{
	dir.cd(settings->value("game_dir").toString());
	if (!dir.entryList().contains("WoW.exe"))
		return;
	if (!dir.cd("WTF"))
		return;
	if (!dir.cd("Account"))
		return;
	
	QFileInfoList lst = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
	foreach(const QFileInfo& inf, lst){
		if (!inf.isDir())
			continue;
		
		//should now be in an account folder
		if (!dir.cd(inf.fileName())){
			continue;
		}
			
		//should now be in.. well, SavedVariables
		if (!dir.cd("SavedVariables")){
			dir.cdUp();
			continue;
		}
			
		if (dir.entryList().contains(data_file_name)){
			ParseLUA(dir.absoluteFilePath(data_file_name));
		}

		dir.cdUp();
		dir.cdUp();
		//should now be back in account folder
	}
}

void MainWindow::ParseLUA(const QString& file_path)
{
	QFile f(file_path);
	if (!f.open(QIODevice::ReadOnly)){
		Information(QString("Unable to open file: %1").arg(file_path));
		return;
	}

	QString all_data = f.readAll();

	all_data = all_data.remove("OnyBuffCollector_buff_table = {");
	all_data = all_data.remove("}");
	all_data = all_data.trimmed();
	
	auto get_val = [](const QString s){
		int start = s.indexOf("\"");
		int stop = s.lastIndexOf("\"");
		return s.mid(start, stop).remove("\"");
	};

	QStringList split_list = all_data.split(",");
	split_list.removeAll("");
	
	for (int i = 0; i < split_list.size(); i += 2){
		if (i + 1 >= split_list.size()){
			continue;
		}
		QString name_str = get_val(split_list[i]);
		if (!name_str.isEmpty()){
			if (!name_str.at(0).isUpper())
				//skipping names that does not start with an upper-case letter,
				//as we won't find them on Realmplayers, and it's probably a bug.
				continue; 
		}
		else{
			continue;
		}
		RequestRealmplayerData(name_str); //send of an asynchronous request for realmplayer data
		QString date_str = get_val(split_list[i + 1]);
		
		QDateTime dt = QDateTime::fromString(date_str, "MM/dd/yy HH:mm:ss");
		dt = dt.addYears(100); //when only yy, it defaults to 19yy instead of 20yy, :(
		if (data_map.contains(name_str))
			continue;


		data_map[name_str] = BuffData(name_str, dt);

	}
}

void MainWindow::RequestRealmplayerData(const QString& name)
{
	
	QUrl url(url_base + name);
	QNetworkRequest req(url);
	net_man->get(req);
}

void MainWindow::UpdateTable()
{
	while (table->rowCount())
		table->removeRow(0);

	for (auto it = data_map.begin(); it != data_map.end(); it++){
		BuffData& d = it.value();
		table->insertRow(0);
		
		table->setItem(0, T_DATE, new QTableWidgetItem(d.date_time.date().toString(Qt::ISODate)));
		table->setItem(0, T_TIME, new QTableWidgetItem(d.date_time.time().toString()));
		table->setItem(0, T_NAME, new QTableWidgetItem(d.name));
		d.guild_itm = new QTableWidgetItem(d.guild);
		table->setItem(0, T_GUILD, d.guild_itm);
	}
	table->resizeColumnsToContents();
	
}
