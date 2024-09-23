package com.byshnev.comport;

import javafx.application.Platform;
import javafx.fxml.FXML;
import javafx.scene.control.*;
import com.fazecast.jSerialComm.*;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyEvent;

import java.util.concurrent.CompletableFuture;

public class ComController {

  @FXML
  private MenuButton bitsInByteMenu;

  @FXML
  private Label bytesCountLabel;

  @FXML
  private TextArea cadresStructureTextArea;

  @FXML
  private TextArea collisionInfoTextArea;

  @FXML
  private MenuButton comNumberMenu;

  @FXML
  private Label baudRateLabel;

  @FXML
  private TextArea inputTextArea;

  @FXML
  private TextArea outputTextArea;

  @FXML
  private Label noAvailablePortsLabel;

  private SerialPort serialPort;  // Текущий порт, в который пишием / читаем
  private boolean portsAvailable = false;

  public void initialize() {
    writeData(); ////!!!!
    byteSizeHandle();
    updateMenu(); ///!!!
    findAvailablePorts();
  }

  public boolean setPort(SerialPort chosenPort){
    // Проверяем, не выбран ли уже текущий порт
    System.out.println("Проверяем, не выбран ли уже текущий порт");
    if (serialPort != null && serialPort.getSystemPortName().equals(chosenPort.getSystemPortName())) {
      return true;  // Порт уже открыт, ничего не делаем
    }
    System.out.println("Попытка открыть порт");
    if (chosenPort.openPort()) {
      if (serialPort != null && serialPort.isOpen()) {
        serialPort.closePort();
      }

      System.out.println("Настраиваем новый порт");
      // Настраиваем новый порт
      serialPort = chosenPort;
      outputTextArea.clear();
      inputTextArea.clear();
      String numberBytes = bitsInByteMenu.getText();
      int byteNumber = Integer.parseInt(numberBytes);
      serialPort.setComPortParameters(9600, byteNumber, 1, 0);

      System.out.println("Перед методом класса Platform");

      // Обновляем интерфейс
      Platform.runLater(() -> comNumberMenu.setText(serialPort.getSystemPortName().substring(3)));
      baudRateLabel.setText(String.valueOf(serialPort.getBaudRate()));

      // Начинаем чтение данных
      readData();
      return true;
    }

    return false;
  }

  public void findAvailablePorts() {
    CompletableFuture.runAsync(() -> {
      while (true) {
        SerialPort[] serialPorts = SerialPort.getCommPorts();
        if (serialPorts.length == 0) {
          comNumberMenu.setVisible(false);
          bitsInByteMenu.setVisible(false);
          noAvailablePortsLabel.setVisible(true);
          portsAvailable=false;
        } else {
          comNumberMenu.setVisible(true);
          bitsInByteMenu.setVisible(true);
          noAvailablePortsLabel.setVisible(false);
          if (!portsAvailable) {
            for(int i = 0; i < serialPorts.length; i++){
              if(setPort(serialPorts[i])) break;
            }
          }
          portsAvailable = true;
        }
      }
    });
  }

  public void updateMenu() {
    comNumberMenu.setOnMouseEntered(event->{
      SerialPort[] serialPorts = SerialPort.getCommPorts();
      comNumberMenu.getItems().clear();
      for(int i = 0; i < serialPorts.length; i++) {
        //String numberOfPort = serialPorts[i].getSystemPortName();
        MenuItem menuItem = new MenuItem(serialPorts[i].getSystemPortName().substring(3));
        menuItem.setOnAction(actionEvent -> {
          SerialPort selectedPort = SerialPort.getCommPort("COM" + menuItem.getText());

          // Проверка перед попыткой подключения
          if (!setPort(selectedPort)) {
            Alert alert = new Alert(Alert.AlertType.WARNING);
            alert.setTitle("Предупреждение");
            alert.setHeaderText(null);
            alert.setContentText("Выбранный порт уже используется другим приложением!");
            alert.show();
          }
        });

        comNumberMenu.getItems().add(menuItem);
      }
    });
  }



  public void writeData(){
    inputTextArea.addEventFilter(KeyEvent.KEY_PRESSED, event->{
      if(event.getCode() == KeyCode.ENTER) {
        if(portsAvailable) {
          byte[] writeBytes = inputTextArea.getText().getBytes();
          inputTextArea.clear();
          serialPort.writeBytes(writeBytes, writeBytes.length);
          Integer writeToStatus = writeBytes.length;
          bytesCountLabel.setText(writeToStatus.toString());
        }
        event.consume();
      }
    });
  }

  public void readData(){
    CompletableFuture.runAsync(() -> {
      while (true) {
        SerialPort tempPort = serialPort;
        byte[] byteBuffer = new byte[9999];
        String tempPortName = tempPort.getSystemPortName();
        String serialPortName = serialPort.getSystemPortName();
        while(tempPortName.equals(serialPortName)){
          while(tempPort.bytesAvailable()>0){
            int readBytes = tempPort.readBytes(byteBuffer,9999);
            if(readBytes!=-1){
              String bytesString = new String(byteBuffer,0,readBytes);
              outputTextArea.setText(bytesString);
            }
          }
        }
      }
    });
  }

  public void byteSizeHandle(){
    bitsInByteMenu.setOnMouseEntered(event ->{
      bitsInByteMenu.getItems().clear();
      for(Integer i = 8; i!=4; i--){
        String number = i.toString();
        MenuItem menuItem = new MenuItem(number);
        menuItem.setOnAction(event1 -> {
          bitsInByteMenu.setText(number);
          if (serialPort!=null) {
            serialPort.setNumDataBits(Integer.parseInt(number));
          }
        });
        bitsInByteMenu.getItems().add(menuItem);
      }
    });
  }

}
