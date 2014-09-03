package com.myapp.otpmasterdegree;

import java.security.NoSuchAlgorithmException;
import java.security.InvalidKeyException;

import android.support.v7.app.ActionBarActivity;
import android.text.format.Time;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.TextView;
import android.widget.ProgressBar;
import android.widget.RadioButton;
import android.widget.Spinner;
import android.widget.ArrayAdapter;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.EditText;

public class MainActivity extends ActionBarActivity {
	
	private boolean TOTP = true;
	private long THIRTY_SECONDS = 30;
	private long ONE_SECOND = 1;
	private int HOW_MANY_OTPS = 3;
	private ProgressBar timeBar = null;
	private CountDownTimer timer = null;
	private EditText editText = null;
	private SharedPreferences sharedPreferences = null;
	private int userId = 0;
	private int USERS = 5;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		timeBar = (ProgressBar) this.findViewById(R.id.time_left_bar);
		((RadioButton) this.findViewById(R.id.radio_totp)).setChecked(true);
		editText = (EditText) this.findViewById(R.id.edit_counter);
		
		// Set up counters from file
		sharedPreferences = getPreferences(Context.MODE_PRIVATE);
		String counterString = sharedPreferences.getString(getString(R.string.counter_values), "0 0 1010 0 1000010");
		String[] counters = counterString.split(" ");
		for (int i = 0; i < counters.length; i++) {
			Secret.setCounterN(i, Long.parseLong(counters[i]));
		}
		
		// Select user
		Spinner spinner = (Spinner) this.findViewById(R.id.users_spinner);
		// Create an ArrayAdapter using the string array and a default spinner layout
		ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(this,
		        R.array.users_array, android.R.layout.simple_spinner_item);
		// Specify the layout to use when the list of choices appears
		adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		// Apply the adapter to the spinner
		spinner.setAdapter(adapter);
		
		spinner.setOnItemSelectedListener(new OnItemSelectedListener() {
			
			@Override
			public void onNothingSelected(AdapterView<?> parent) {
				
			}
			
			@Override
			public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
				String user = (String) parent.getItemAtPosition(pos);
				String[] numbers = user.split(" ");
				userId = Integer.parseInt(numbers[1]) - 1;
				editText.setText(String.valueOf(Secret.getCounterN(userId)));
				startNewTimer();
			}
		});
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		//getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		int id = item.getItemId();
		if (id == R.id.action_settings) {
			return true;
		}
		return super.onOptionsItemSelected(item);
	}
	
	/**
	 * Called when user clicks on button.
	 * @param view
	 */
	public void onButtonClickedShowOTPs(View view) {
		if (TOTP) {
			if (timer == null) {
				startNewTimer();
			}
		}
		else {
			startNewTimer();
		}
		
	}
	
	/**
	 * Called when user clicks on radio button.
	 */
	public void onRadioButtonClicked(View view) {
		boolean checked = ((RadioButton) view).isChecked();
		switch (view.getId()) {
		case R.id.radio_totp:
			if (checked) {
				if ((timer == null) || !TOTP) {
					TOTP = true;
					startNewTimer();
				}
			}
			break;
		case R.id.radio_hotp:
			if (checked) {
				TOTP = false;
				startNewTimer();
			}
			break;
		}
	}
	
	/**
	 * Generates password and shows them on screen.
	 * @param secondsFromEpochStep time value for TOTP
	 */
	private void getOTPsSetOnScreen(long secondsFromEpochStep)
		throws NoSuchAlgorithmException, InvalidKeyException {
		StringBuffer otps = new StringBuffer();
		if (TOTP) {
			int secondsOffset = HOW_MANY_OTPS / 2 * (-1);
			for (int i = 0; i < HOW_MANY_OTPS; i++) {
				long t = secondsFromEpochStep + secondsOffset;
				String s = OneTimePasswordAlgorithm.generateOTP(Secret.getSeedN(userId), t, 6, false, -1);
				if (i == (HOW_MANY_OTPS/2)) {
					s = new String(">" + s + "<");
				}
				else {
					s = new String("  " + s + "  ");
				}
				otps.append(s);
				otps.append("\n");
				secondsOffset++;
			}
		}
		else {
			int i = 0;
			long counterOffset = HOW_MANY_OTPS / 2 * (-1);
			long counter;
			try {
				counter = Integer.parseInt(editText.getText().toString());
			}
			catch (NumberFormatException exc) {
				counter = Secret.getCounterN(userId);
			}
			editText.setText(String.valueOf(counter+1));
			Secret.setCounterN(userId, counter+1);
			// Save counters to file
			StringBuffer sb = new StringBuffer();
			for (int j = 0; j < USERS; j++) {
				sb.append(Secret.getCounterN(j));
				sb.append(" ");
			}
			String sCounters = sb.substring(0, sb.length()-1);
			SharedPreferences.Editor editor = sharedPreferences.edit();
			editor.putString(getString(R.string.counter_values), sCounters);
			editor.commit();
			
			if (counter == 0) {
				otps.append("\n");
				counterOffset++;
				i = 1;
			}
			for ( ; i < HOW_MANY_OTPS; i++) {
				long t = counter + counterOffset; 
				String s = OneTimePasswordAlgorithm.generateOTP(Secret.getSeedN(userId), t, 6, false, -1);
				if (i == (HOW_MANY_OTPS/2)) {
					s = new String(">" + s + "<");
				}
				else {
					s = new String("  " + s + "  ");
				}
				otps.append(s);
				otps.append("\n");
				counterOffset++;
			}
		}
		TextView tv = (TextView) this.findViewById(R.id.view_otps);
		tv.setText(otps);
	}
	
	/**
	 * Starts new timer.
	 */
	private void startNewTimer() {
		if (timer != null) {
			timer.cancel();
		}
		Time time = new Time();
		time.setToNow();
		long milliSeconds = 0;
		if (TOTP) {
			milliSeconds = (30 - time.second%30) * 1000;
		}
		else {
			milliSeconds = 30 * 1000;
		}
		
		long secondsFromEpochStep = time.toMillis(false)/(THIRTY_SECONDS*1000);
		timeBar.setProgress((int)((milliSeconds/10)/THIRTY_SECONDS));
		
		try {
			getOTPsSetOnScreen(secondsFromEpochStep);
		}
		catch (NoSuchAlgorithmException exc) {
			
		}
		catch (InvalidKeyException exc) {
			
		}
		
		timer = new CountDownTimer(milliSeconds, ONE_SECOND) {
			public void onTick(long millisUntilFinished) {
				timeBar.setProgress((int)((millisUntilFinished/10)/THIRTY_SECONDS));
			}
			
			public void onFinish() {
				if (TOTP) {
					startNewTimer();
				}
				else {
					
				}
			}
		};
		timer.start();
	}
}
